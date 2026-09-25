#include "android_jni.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_system.h>

#include <jni.h>

#include <cstdio>

namespace mhp3rd::android {
namespace {

// The activity's own class: a thread the JVM did not start cannot find app
// classes by name, but the activity object knows its class.
struct Call {
    JNIEnv *env{};
    jclass type{};
    Call() {
        env = static_cast<JNIEnv *>(SDL_GetAndroidJNIEnv());
        if (env == nullptr) return;
        auto activity = static_cast<jobject>(SDL_GetAndroidActivity());
        if (activity == nullptr) return;
        type = env->GetObjectClass(activity);
        env->DeleteLocalRef(activity);
    }
    ~Call() {
        if (env != nullptr && type != nullptr) env->DeleteLocalRef(type);
    }
    Call(const Call &) = delete;
    Call &operator=(const Call &) = delete;
    [[nodiscard]] bool ok() const { return env != nullptr && type != nullptr; }
    jmethodID method(const char *name, const char *signature) const {
        jmethodID id = env->GetStaticMethodID(type, name, signature);
        if (id == nullptr) env->ExceptionClear();
        return id;
    }
    // Clears a pending Java exception; true if there was one.
    bool failed() const {
        if (!env->ExceptionCheck()) return false;
        env->ExceptionClear();
        return true;
    }
    [[nodiscard]] std::optional<std::string> text(jobject object) const {
        if (object == nullptr) return std::nullopt;
        auto string = static_cast<jstring>(object);
        const char *chars = env->GetStringUTFChars(string, nullptr);
        std::optional<std::string> result;
        if (chars != nullptr) result = std::string(chars);
        env->ReleaseStringUTFChars(string, chars);
        env->DeleteLocalRef(object);
        return result;
    }
    jstring string(const std::string &value) const { return env->NewStringUTF(value.c_str()); }
};

} // namespace

Insets cutout_insets() {
    Insets insets;
    Call call;
    if (!call.ok()) return insets;
    jmethodID id = call.method("cutoutInsets", "()[I");
    if (id == nullptr) return insets;
    auto array = static_cast<jintArray>(call.env->CallStaticObjectMethod(call.type, id));
    if (call.failed() || array == nullptr) return insets;
    jint values[4]{};
    if (call.env->GetArrayLength(array) == 4) call.env->GetIntArrayRegion(array, 0, 4, values);
    call.env->DeleteLocalRef(array);
    return {values[0], values[1], values[2], values[3]};
}

void relaunch() {
    Call call;
    if (!call.ok()) return;
    jmethodID id = call.method("relaunch", "()V");
    if (id == nullptr) return;
    std::fflush(stdout);
    std::fflush(stderr);
    call.env->CallStaticVoidMethod(call.type, id);
    call.failed();
}

std::optional<std::string> pick_folder() {
    Call call;
    if (!call.ok()) return std::nullopt;
    jmethodID id = call.method("pickFolder", "()Ljava/lang/String;");
    if (id == nullptr) return std::nullopt;
    jobject result = call.env->CallStaticObjectMethod(call.type, id);
    if (call.failed()) return std::nullopt;
    return call.text(result);
}

std::optional<std::string> pick_document() {
    Call call;
    if (!call.ok()) return std::nullopt;
    jmethodID id = call.method("pickDocument", "()Ljava/lang/String;");
    if (id == nullptr) return std::nullopt;
    jobject result = call.env->CallStaticObjectMethod(call.type, id);
    if (call.failed()) return std::nullopt;
    return call.text(result);
}

std::string tree_root(const std::string &tree_uri) {
    Call call;
    if (!call.ok()) return {};
    jmethodID id = call.method("treeRoot", "(Ljava/lang/String;)Ljava/lang/String;");
    if (id == nullptr) return {};
    jstring argument = call.string(tree_uri);
    jobject result = call.env->CallStaticObjectMethod(call.type, id, argument);
    call.env->DeleteLocalRef(argument);
    if (call.failed()) return {};
    return call.text(result).value_or(std::string());
}

std::optional<std::vector<Entry>> list_folder(const std::string &folder_uri) {
    Call call;
    if (!call.ok()) return std::nullopt;
    jmethodID id = call.method("listFolder", "(Ljava/lang/String;)[Ljava/lang/String;");
    if (id == nullptr) return std::nullopt;
    jstring argument = call.string(folder_uri);
    auto array = static_cast<jobjectArray>(call.env->CallStaticObjectMethod(call.type, id, argument));
    call.env->DeleteLocalRef(argument);
    if (call.failed() || array == nullptr) return std::nullopt;
    std::vector<Entry> entries;
    const jsize count = call.env->GetArrayLength(array);
    for (jsize i = 0; i < count; ++i) {
        const std::optional<std::string> line = call.text(call.env->GetObjectArrayElement(array, i));
        if (!line || line->size() < 4u) continue;
        const std::size_t name_end = line->find('/', 2u);
        if (name_end == std::string::npos) continue;
        entries.push_back({(*line)[0] == 'd', line->substr(2u, name_end - 2u), line->substr(name_end + 1u)});
    }
    call.env->DeleteLocalRef(array);
    return entries;
}

std::optional<std::string> create(const std::string &folder_uri, const std::string &name, bool directory) {
    Call call;
    if (!call.ok()) return std::nullopt;
    jmethodID id = call.method("create", "(Ljava/lang/String;Ljava/lang/String;Z)Ljava/lang/String;");
    if (id == nullptr) return std::nullopt;
    jstring folder = call.string(folder_uri);
    jstring file = call.string(name);
    jobject result = call.env->CallStaticObjectMethod(call.type, id, folder, file, directory ? JNI_TRUE : JNI_FALSE);
    call.env->DeleteLocalRef(folder);
    call.env->DeleteLocalRef(file);
    if (call.failed()) return std::nullopt;
    return call.text(result);
}

int open_document(const std::string &uri, const char *mode) {
    Call call;
    if (!call.ok()) return -1;
    jmethodID id = call.method("openDocument", "(Ljava/lang/String;Ljava/lang/String;)I");
    if (id == nullptr) return -1;
    jstring document = call.string(uri);
    jstring open_mode = call.env->NewStringUTF(mode);
    const jint fd = call.env->CallStaticIntMethod(call.type, id, document, open_mode);
    call.env->DeleteLocalRef(document);
    call.env->DeleteLocalRef(open_mode);
    if (call.failed()) return -1;
    return fd;
}

} // namespace mhp3rd::android
