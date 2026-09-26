"""
NDS Model Exporter for Blender
Author: P3D Team
Version: 2.1.4 (Bake World Space Geometry + Texture Name Sync + Axis Fix + Blender 5.x Support)
Blender: 3.x / 4.x / 5.x
"""

import bpy
import bmesh
import io
import json
import os
import re
import tempfile
import zipfile
import math
import mathutils
from collections import defaultdict
from bpy.props import StringProperty, BoolProperty
from bpy_extras.io_utils import ExportHelper

bl_info = {
    "name": "NDS Model Exporter",
    "author": "P3D Team",
    "version": (2, 1, 4),
    "blender": (3, 0, 0),
    "location": "File > Export > NDS Model (.zip)",
    "description": "Exports armature + mesh baked into Blender World Space",
    "category": "Import-Export",
}


def sanitize(name: str) -> str:
    return re.sub(r"[^a-zA-Z0-9]", "_", name)


def bone_index(armature, bone_name: str, bone_list: list) -> int:
    for i, b in enumerate(bone_list):
        if b.name == bone_name:
            return i
    return -1


def parent_index(bone, bone_list: list) -> int:
    if bone.parent is None:
        return -1
    return bone_index(None, bone.parent.name, bone_list)


def get_material_image(mat):
    if mat is None or not mat.use_nodes:
        return None

    # 1. Try to trace the Principled BSDF Base Color (Most accurate)
    for node in mat.node_tree.nodes:
        if node.type == "BSDF_PRINCIPLED":
            color_socket = node.inputs.get("Base Color")
            if color_socket and color_socket.is_linked:
                src_node = color_socket.links[0].from_node
                if src_node.type == "TEX_IMAGE" and src_node.image:
                    return src_node.image

    # 2. Fallback: Find ANY Image Texture that is linked to a shader
    for node in mat.node_tree.nodes:
        if node.type == "TEX_IMAGE" and node.image:
            if getattr(node.outputs[0], "is_linked", False):
                return node.image

    # 3. Final Resort: Just grab the first image node that exists
    for node in mat.node_tree.nodes:
        if node.type == "TEX_IMAGE" and node.image:
            return node.image

    return None


def get_export_filename(image, model_name: str, name_map: dict) -> str:
    if image.name not in name_map:
        name_map[image.name] = f"{model_name}_texture_{len(name_map)}.png"
    return name_map[image.name]


def image_to_png_bytes(image) -> bytes:
    tmp_path = tempfile.mktemp(suffix=".png")
    try:
        orig_path, orig_format = image.filepath_raw, image.file_format
        image.filepath_raw, image.file_format = tmp_path, "PNG"
        image.save()
        image.filepath_raw, image.file_format = orig_path, orig_format
        with open(tmp_path, "rb") as f:
            return f.read()
    finally:
        if os.path.exists(tmp_path):
            os.unlink(tmp_path)


def build_mtl(mat_to_image: dict, model_name: str, name_map: dict) -> str:
    lines = ["# NDS MTL export"]
    for mat_name, image in mat_to_image.items():
        lines.append(f"\nnewmtl {mat_name}")
        if image:
            new_name = get_export_filename(image, model_name, name_map)
            lines.append(f"map_Kd {new_name}")
    return "\n".join(lines) + "\n"


def export_bone_obj(
    arm_obj, mesh_objects, bone, depsgraph, obj_name: str, correction: mathutils.Matrix
):
    mtllib_name = obj_name + ".mtl"
    lines = ["# NDS OBJ export", f"# bone: {bone.name}", "", f"mtllib {mtllib_name}"]
    vert_lines, uv_lines, face_groups = [], [], []
    global_v_offset = 1
    mat_to_image = {}

    for mesh_obj in mesh_objects:
        if bone.name not in mesh_obj.vertex_groups:
            continue
        vg_index = mesh_obj.vertex_groups[bone.name].index
        eval_obj = mesh_obj.evaluated_get(depsgraph)
        eval_mesh = eval_obj.to_mesh()

        bm = bmesh.new()
        bm.from_mesh(eval_mesh)
        bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 4])
        bm.to_mesh(eval_mesh)
        bm.free()

        eval_mesh.calc_loop_triangles()
        uv_layer = eval_mesh.uv_layers.active
        mats = eval_mesh.materials

        bone_verts = set()
        faces_to_export = []
        for poly in eval_mesh.polygons:
            weights = defaultdict(float)
            for li in poly.loop_indices:
                v = eval_mesh.vertices[eval_mesh.loops[li].vertex_index]
                for g in v.groups:
                    weights[g.group] += g.weight
            if not weights:
                continue
            if max(weights, key=weights.get) == vg_index:
                faces_to_export.append(poly)
                for li in poly.loop_indices:
                    bone_verts.add(eval_mesh.loops[li].vertex_index)

        if not faces_to_export:
            eval_obj.to_mesh_clear()
            continue

        local_verts = {}
        for vi in sorted(bone_verts):
            # Apply the correction matrix to the verticies
            world_co = correction @ mesh_obj.matrix_world @ eval_mesh.vertices[vi].co
            vert_lines.append(f"v {world_co.x:.6f} {world_co.y:.6f} {world_co.z:.6f}")
            local_verts[vi] = global_v_offset
            global_v_offset += 1

        uv_map, uv_counter = {}, len(uv_lines) + 1
        mat_face_map = defaultdict(list)

        for poly in faces_to_export:
            mat = mats[poly.material_index] if poly.material_index < len(mats) else None
            mat_name = sanitize(mat.name) if mat else "__no_material__"
            if mat_name not in mat_to_image:
                mat_to_image[mat_name] = get_material_image(mat)

            face_v = []
            for li in poly.loop_indices:
                vi = eval_mesh.loops[li].vertex_index
                if uv_layer:
                    uv = uv_layer.data[li].uv
                    key = (round(uv.x, 5), round(uv.y, 5))
                    if key not in uv_map:
                        uv_lines.append(f"vt {uv.x:.6f} {uv.y:.6f}")
                        uv_map[key] = uv_counter
                        uv_counter += 1
                    face_v.append(f"{local_verts[vi]}/{uv_map[key]}")
                else:
                    face_v.append(f"{local_verts[vi]}")
            if len(face_v) >= 3:
                mat_face_map[mat_name].append("f " + " ".join(face_v))

        for mat_name, faces in mat_face_map.items():
            face_groups.append((mat_name, faces))
        eval_obj.to_mesh_clear()

    if not vert_lines:
        return "# Empty Node\n", {}
    lines += vert_lines + uv_lines
    for mat_name, faces in face_groups:
        lines.append(f"usemtl {mat_name}")
        lines.extend(faces)
    return "\n".join(lines) + "\n", mat_to_image


def get_bone_transforms_at_frame(
    arm_obj, bone_name, frame, scene, correction: mathutils.Matrix
):
    scene.frame_set(frame)
    pose_bone = arm_obj.pose.bones.get(bone_name)
    bone = arm_obj.data.bones.get(bone_name)

    if pose_bone is None or bone is None:
        return (0, 0, 0), (0, 0, 0)

    # Apply the correction matrix to the armature world matrix
    mw = correction @ arm_obj.matrix_world

    world_pose_n = mw @ pose_bone.matrix
    world_rest_n = mw @ bone.matrix_local
    delta_n = world_pose_n @ world_rest_n.inverted()

    if bone.parent:
        pose_parent = arm_obj.pose.bones[bone.parent.name]
        bone_parent = arm_obj.data.bones[bone.parent.name]
        world_pose_p = mw @ pose_parent.matrix
        world_rest_p = mw @ bone_parent.matrix_local
        delta_p = world_pose_p @ world_rest_p.inverted()
    else:
        delta_p = mathutils.Matrix.Identity(4)

    rel = delta_p.inverted() @ delta_n

    origin_n_w = mw @ bone.head_local
    T_orig = mathutils.Matrix.Translation(origin_n_w)
    T_orig_inv = mathutils.Matrix.Translation(-origin_n_w)

    M = T_orig_inv @ rel @ T_orig

    loc, rot_q, _scale = M.decompose()
    rot_e = rot_q.to_euler("XYZ")

    return (math.degrees(rot_e.x), math.degrees(rot_e.y), math.degrees(rot_e.z)), (
        loc.x,
        loc.y,
        loc.z,
    )


def collect_animations(arm_obj, bone_list, scene, correction: mathutils.Matrix):
    animations, original_frame = {}, scene.frame_current

    def export_action(action, anim_name):
        f_start, f_end = float("inf"), float("-inf")
        target_fcurves = []

        # Determine the correct F-Curve path based on the Blender version
        if hasattr(action, "slots") and len(action.slots) > 0:
            # Blender 5.0+ Slotted API
            for layer in getattr(action, "layers", []):
                for strip in getattr(layer, "strips", []):
                    # Try to grab frame ranges from the strips if they exist
                    if hasattr(strip, "frame_start"):
                        f_start = min(f_start, strip.frame_start)
                        f_end = max(f_end, strip.frame_end)

                    for slot in action.slots:
                        if hasattr(strip, "channelbag"):
                            bag = strip.channelbag(slot)
                            if bag and getattr(bag, "fcurves", None):
                                target_fcurves.extend(bag.fcurves)
        elif hasattr(action, "fcurves"):
            # Blender 4.x and below Legacy API
            target_fcurves = action.fcurves
            if hasattr(action, "frame_range"):
                f_start, f_end = action.frame_range[0], action.frame_range[1]

        # Fallback: If frame range wasn't found, calculate it from the keyframes
        if f_start == float("inf"):
            for fcurve in target_fcurves:
                for kp in getattr(fcurve, "keyframe_points", []):
                    f_start = min(f_start, kp.co[0])
                    f_end = max(f_end, kp.co[0])

            if f_start == float("inf"):
                f_start, f_end = 0, 0

        f_start, f_end = int(f_start), int(f_end)

        anim_data = {"duration": f_end - f_start + 1, "tracks": {}}
        bone_frames = defaultdict(set)

        for fcurve in target_fcurves:
            if not fcurve.data_path.startswith("pose.bones"):
                continue
            try:
                bname = fcurve.data_path.split('"')[1]
            except IndexError:
                continue
            for kp in fcurve.keyframe_points:
                bone_frames[bname].add(int(round(kp.co[0])))

        for bone in bone_list:
            if bone.name not in bone_frames:
                continue
            bid = bone_index(None, bone.name, bone_list)
            track = []
            for f in sorted(bone_frames[bone.name]):
                rot, pos = get_bone_transforms_at_frame(
                    arm_obj, bone.name, f, scene, correction
                )
                track.append({"time": f - f_start, "rot": list(rot), "pos": list(pos)})
            if track:
                anim_data["tracks"][str(bid)] = track

        animations[sanitize(anim_name)] = anim_data

    if not arm_obj.animation_data:
        arm_obj.animation_data_create()

    orig_action = getattr(arm_obj.animation_data, "action", None)
    orig_slot = getattr(arm_obj.animation_data, "action_slot", None)

    for action in bpy.data.actions:
        arm_obj.animation_data.action = action
        # Blender 5.x requires assigning an action slot for the animation to play
        if (
            hasattr(arm_obj.animation_data, "action_suitable_slots")
            and arm_obj.animation_data.action_suitable_slots
        ):
            arm_obj.animation_data.action_slot = (
                arm_obj.animation_data.action_suitable_slots[0]
            )

        export_action(action, action.name)

    # Restore original state
    arm_obj.animation_data.action = orig_action
    if orig_slot and hasattr(arm_obj.animation_data, "action_slot"):
        arm_obj.animation_data.action_slot = orig_slot

    scene.frame_set(original_frame)
    return animations


class NDS_OT_ExportModel(bpy.types.Operator, ExportHelper):
    bl_idname, bl_label, bl_options = (
        "export_scene.nds_model",
        "Export NDS Model",
        {"PRESET"},
    )
    filename_ext = ".zip"
    filter_glob: StringProperty(default="*.zip", options={"HIDDEN"})

    # UI toggle to turn on/off axis fix on or off
    fix_nds_axis: BoolProperty(
        name="Fix NDS Axis (Rotate 180 Z)",
        description="Automatically fixes left/right & forwards/backwards issues for the NDS engine.",
        default=True,
    )

    def execute(self, context):
        scene = context.scene

        # Determine if we need to apply the 180 degree rotation mathematically
        if self.fix_nds_axis:
            correction = mathutils.Matrix.Rotation(math.radians(180.0), 4, "Z")
        else:
            correction = mathutils.Matrix.Identity(4)

        arm_obj = next(
            (o for o in scene.objects if o.type == "ARMATURE" and o.select_get()), None
        )
        if not arm_obj:
            arm_obj = next((o for o in scene.objects if o.type == "ARMATURE"), None)
        if not arm_obj:
            self.report({"ERROR"}, "No armature found.")
            return {"CANCELLED"}

        mesh_objects = [
            o
            for o in scene.objects
            if o.type == "MESH"
            and (
                o.parent == arm_obj
                or any(
                    m.type == "ARMATURE" and m.object == arm_obj for m in o.modifiers
                )
            )
        ]

        model_name = sanitize(os.path.splitext(os.path.basename(self.filepath))[0])
        arm_obj.data.pose_position = "REST"
        context.view_layer.update()
        depsgraph = context.evaluated_depsgraph_get()
        bone_list = list(arm_obj.data.bones)

        zip_buffer = io.BytesIO()
        with zipfile.ZipFile(zip_buffer, "w", zipfile.ZIP_DEFLATED) as zf:
            nodes, all_tex_images = [], {}
            texture_name_map = {}

            for i, bone in enumerate(bone_list):
                obj_name = f"{model_name}_n{i}"

                # Pass the correction matrix to the geometry builder
                obj_content, mat_to_image = export_bone_obj(
                    arm_obj, mesh_objects, bone, depsgraph, obj_name, correction
                )
                zf.writestr(f"{obj_name}.obj", obj_content)
                zf.writestr(
                    f"{obj_name}.mtl",
                    build_mtl(mat_to_image, model_name, texture_name_map),
                )

                for img in mat_to_image.values():
                    if img:
                        tex_name = get_export_filename(
                            img, model_name, texture_name_map
                        )
                        all_tex_images[tex_name] = img

                # Apply the correction matrix to the bone origin
                world_head = correction @ arm_obj.matrix_world @ bone.head_local
                nodes.append(
                    {
                        "id": i,
                        "parent": parent_index(bone, bone_list),
                        "name": bone.name,
                        "obj": f"{obj_name}.obj",
                        "origin": list(world_head),
                    }
                )

            for tex_fn, image in all_tex_images.items():
                try:
                    zf.writestr(tex_fn, image_to_png_bytes(image))
                except Exception as e:
                    self.report({"WARNING"}, f"Texture error: {e}")

            arm_obj.data.pose_position = "POSE"
            # Pass the correction matrix to the animation compiler
            anims = collect_animations(arm_obj, bone_list, scene, correction)
            arm_obj.data.pose_position = "REST"

            zf.writestr(
                f"{model_name}.json",
                json.dumps({"nodes": nodes, "animations": anims}, indent=2),
            )

        zip_buffer.seek(0)
        with open(self.filepath, "wb") as f:
            f.write(zip_buffer.read())
        return {"FINISHED"}


def menu_func_export(self, context):
    self.layout.operator(NDS_OT_ExportModel.bl_idname, text="NDS Model (.zip)")


def register():
    bpy.utils.register_class(NDS_OT_ExportModel)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(NDS_OT_ExportModel)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()
