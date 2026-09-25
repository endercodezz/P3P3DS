package io.github.teamgdb.yakumo;

import android.content.ContentResolver;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.database.Cursor;
import android.graphics.Insets;
import android.net.Uri;
import android.os.Build;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract;
import android.view.DisplayCutout;
import android.view.View;
import android.view.WindowInsets;

import org.libsdl.app.SDLActivity;

import java.util.ArrayList;

/**
 * SDL's activity with what the host needs from Android and SDL does not
 * offer: the display cutout alone (SDL's safe area also counts the gesture
 * areas of hidden system bars), and the system's document picker for whole
 * folders, with the documents in them read and written through file
 * descriptors. The native side calls the static methods through JNI from the
 * game's thread; each picker call blocks that thread until the player has
 * chosen, never the UI thread.
 */
public class YakumoActivity extends SDLActivity {
    private static final int kPickTree = 0x59414b01;
    private static final Object sPickLock = new Object();
    private static boolean sPickDone;
    private static String sPickResult;

    /**
     * Keeps the game in landscape, either way up, as the manifest asks. SDL
     * calls this when it makes the window and would otherwise ask for every
     * orientation (FULL_USER) for a resizable window without an orientations
     * hint: the game then turned to portrait whenever the phone was held
     * upright, and stayed there on a phone with auto-rotate off.
     */
    @Override
    public void setOrientationBis(int w, int h, boolean resizable, String hint) {
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
    }

    /** Left, top, right, bottom of the display cutout in window pixels, or all 0. */
    public static int[] cutoutInsets() {
        int[] result = new int[4];
        if (mSingleton == null) return result;
        View view = mSingleton.getWindow().getDecorView();
        WindowInsets insets = view.getRootWindowInsets();
        if (insets == null) return result;
        if (Build.VERSION.SDK_INT >= 30) {
            Insets cutout = insets.getInsets(WindowInsets.Type.displayCutout());
            result[0] = cutout.left;
            result[1] = cutout.top;
            result[2] = cutout.right;
            result[3] = cutout.bottom;
        } else {
            // Android 10: the same insets through the older call.
            DisplayCutout cutout = insets.getDisplayCutout();
            if (cutout == null) return result;
            result[0] = cutout.getSafeInsetLeft();
            result[1] = cutout.getSafeInsetTop();
            result[2] = cutout.getSafeInsetRight();
            result[3] = cutout.getSafeInsetBottom();
        }
        return result;
    }

    /** Asks the player for a folder; its tree URI, or null when cancelled. */
    public static String pickFolder() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        return pick(intent);
    }

    /** Asks the player for a file to read; its document URI, or null when cancelled. */
    public static String pickDocument() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        // Disc images have no MIME type every provider agrees on.
        intent.setType("*/*");
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        return pick(intent);
    }

    private static String pick(Intent intent) {
        if (mSingleton == null) return null;
        synchronized (sPickLock) {
            sPickDone = false;
            sPickResult = null;
        }
        mSingleton.runOnUiThread(() -> {
            try {
                mSingleton.startActivityForResult(intent, kPickTree);
            } catch (Exception e) {
                finishPick(null);
            }
        });
        synchronized (sPickLock) {
            while (!sPickDone) {
                try {
                    sPickLock.wait();
                } catch (InterruptedException e) {
                    return null;
                }
            }
            return sPickResult;
        }
    }

    private static void finishPick(String result) {
        synchronized (sPickLock) {
            sPickResult = result;
            sPickDone = true;
            sPickLock.notifyAll();
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == kPickTree) {
            Uri uri = resultCode == RESULT_OK && data != null ? data.getData() : null;
            finishPick(uri != null ? uri.toString() : null);
            return;
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    /** Starts the app afresh: this process ends and a new one opens the activity. */
    public static void relaunch() {
        if (mSingleton == null) return;
        Intent launch = mSingleton.getPackageManager().getLaunchIntentForPackage(mSingleton.getPackageName());
        if (launch == null || launch.getComponent() == null) return;
        mSingleton.startActivity(Intent.makeRestartActivityTask(launch.getComponent()));
        Runtime.getRuntime().exit(0);
    }

    /** The document that stands for a picked tree itself. */
    public static String treeRoot(String treeUri) {
        Uri tree = Uri.parse(treeUri);
        return DocumentsContract.buildDocumentUriUsingTree(tree, DocumentsContract.getTreeDocumentId(tree))
            .toString();
    }

    /**
     * The children of a folder document in a picked tree, as "d/name/uri" for
     * folders and "f/name/uri" for files (names cannot hold '/').
     */
    public static String[] listFolder(String folderUri) {
        ArrayList<String> entries = new ArrayList<>();
        Uri folder = Uri.parse(folderUri);
        Uri children = DocumentsContract.buildChildDocumentsUriUsingTree(folder,
            DocumentsContract.getDocumentId(folder));
        ContentResolver resolver = mSingleton.getContentResolver();
        String[] columns = {DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                            DocumentsContract.Document.COLUMN_DISPLAY_NAME,
                            DocumentsContract.Document.COLUMN_MIME_TYPE};
        try (Cursor cursor = resolver.query(children, columns, null, null, null)) {
            while (cursor != null && cursor.moveToNext()) {
                String id = cursor.getString(0);
                String name = cursor.getString(1);
                boolean directory = DocumentsContract.Document.MIME_TYPE_DIR.equals(cursor.getString(2));
                Uri child = DocumentsContract.buildDocumentUriUsingTree(folder, id);
                entries.add((directory ? "d/" : "f/") + name + "/" + child);
            }
        } catch (Exception e) {
            return null;
        }
        return entries.toArray(new String[0]);
    }

    /** Creates a folder ("vnd.android.document/directory") or a file in a folder document. */
    public static String create(String folderUri, String name, boolean directory) {
        try {
            Uri created = DocumentsContract.createDocument(mSingleton.getContentResolver(), Uri.parse(folderUri),
                directory ? DocumentsContract.Document.MIME_TYPE_DIR : "application/octet-stream", name);
            return created != null ? created.toString() : null;
        } catch (Exception e) {
            return null;
        }
    }

    /** A file descriptor the caller owns for a document, mode "r" or "w", or -1. */
    public static int openDocument(String uri, String mode) {
        try {
            ParcelFileDescriptor descriptor =
                mSingleton.getContentResolver().openFileDescriptor(Uri.parse(uri), mode.equals("w") ? "wt" : mode);
            return descriptor != null ? descriptor.detachFd() : -1;
        } catch (Exception e) {
            return -1;
        }
    }
}
