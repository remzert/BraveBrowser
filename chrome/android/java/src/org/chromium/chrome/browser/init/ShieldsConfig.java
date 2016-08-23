/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.init;

import android.content.Context;
import android.os.AsyncTask;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.HashMap;
import java.util.concurrent.locks.ReentrantReadWriteLock;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

@JNINamespace("net::blockers")
public class ShieldsConfig {

    private static final String TAG = "ShieldsConfig";
    private static final String SHIELDS_CONFIG_LOCALFILENAME = "shields_config.dat";
    private HashMap<String, String> mSettings = new HashMap<String, String>();
    private ReentrantReadWriteLock mLock = new ReentrantReadWriteLock();
    private Context mContext = null;


    public ShieldsConfig() {
        mContext = ContextUtils.getApplicationContext();
        nativeInit();
        new ReadDataAsyncTask().execute();
    }

    class ReadDataAsyncTask extends AsyncTask<Void,Void,Long> {
        protected Long doInBackground(Void... params) {
            try {
                File dataPath = new File(mContext.getApplicationInfo().dataDir, SHIELDS_CONFIG_LOCALFILENAME);

                byte[] buffer = null;
                if (dataPath.exists()) {
                    buffer = ADBlockUtils.readLocalFile(dataPath);
                }
                if (null != buffer) {
                    String[] array = new String(buffer).split(";");
                    for (int i = 0; i < array.length; i++) {
                        if (array[i].equals("")) {
                            continue;
                        }
                        int splitterIndex = array[i].indexOf(",");
                        String host = "";
                        String settings = "";
                        if (-1 != splitterIndex) {
                            host = array[i].substring(0, splitterIndex);
                            if (array[i].length() > splitterIndex + 1) {
                                settings = array[i].substring(splitterIndex + 1);
                            }
                        }
                        if (0 != host.length() && 0 != settings.length()) {
                            mSettings.put(host, settings);
                        }
                    }
                }
            }
            catch (Exception exc) {
                Log.i(TAG, "Error on reading dat file.");
            }
            return null;
        }
    }

    public void Close() {
        nativeClear();
    }

    public void setTopHost(String host, boolean enabled) {
        if (null != host && host.startsWith("www.")) {
            host = host.substring("www.".length());
        }
        try {
            mLock.writeLock().lock();
            if (!enabled) {
                mSettings.put(host, "0");
            } else {
                mSettings.remove(host);
            }
        }
        finally {
            mLock.writeLock().unlock();
        }
        new SaveDataAsyncTask().execute();
    }

    public boolean isTopShieldsEnabled(String host) {
        String settings = getHostSettings(host);
        if (null != settings && 0 != settings.length() && '0' == settings.charAt(0)) {
            return false;
        }

        return true;
    }

    class SaveDataAsyncTask extends AsyncTask<Void,Void,Long> {
        protected Long doInBackground(Void... params) {
            saveSettings();

            return null;
        }
    }

    private void saveSettings() {
      File dataPath = new File(mContext.getApplicationInfo().dataDir, SHIELDS_CONFIG_LOCALFILENAME);
        try {
            FileOutputStream outputStream = new FileOutputStream(dataPath);
            boolean firstIteration = true;
            mLock.writeLock().lock();
            for (HashMap.Entry<String, String> entry : mSettings.entrySet()) {
                String key = entry.getKey();
                String value = entry.getValue();
                if (!firstIteration) {
                    outputStream.write(";".getBytes(), 0, ";".length());
                }
                outputStream.write(key.getBytes(), 0, key.length());
                outputStream.write(",".getBytes(), 0, ",".length());
                outputStream.write(value.getBytes(), 0, value.length());
                firstIteration = false;
            }
            outputStream.close();
        }
        catch (FileNotFoundException e) {
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        finally {
            mLock.writeLock().unlock();
        }
    }

    @CalledByNative
    public String getHostSettings(String host) {
        if (null != host && host.startsWith("www.")) {
            host = host.substring("www.".length());
        }
        try {
            mLock.readLock().lock();

            String settings = mSettings.get(host);
            if (null != settings) {
                return settings;
            }
        }
        finally {
            mLock.readLock().unlock();
        }

        return "1";
    }

    private native void nativeInit();
    private native void nativeClear();
}
