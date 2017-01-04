/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.init;

import android.content.Context;
import android.content.SharedPreferences;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.Calendar;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipEntry;

import org.chromium.base.Log;
import org.chromium.base.PathUtils;

public class ADBlockUtils {

    public static final String PRIVATE_DATA_DIRECTORY_SUFFIX = "chrome";

    public static final String TRACKING_PROTECTION_URL = "https://s3.amazonaws.com/tracking-protection-data/1/TrackingProtection.dat";
    public static final String TRACKING_PROTECTION_LOCALFILENAME = "TrackingProtection.dat";
    public static final String TRACKING_PROTECTION_LOCALFILENAME_DOWNLOADED = "TrackingProtectionDownloaded.dat";
    public static final String ETAG_PREPEND_TP = "tp";

    public static final String ADBLOCK_URL = "https://s3.amazonaws.com/adblock-data/2/ABPFilterParserData.dat";
    public static final String ADBLOCK_LOCALFILENAME = "ABPFilterParserData.dat";
    public static final String ADBLOCK_LOCALFILENAME_DOWNLOADED = "ABPFilterParserDataDownloaded.dat";
    public static final String ETAG_PREPEND_ADBLOCK = "abp";

    public static final String HTTPS_URL = "https://s3.amazonaws.com/https-everywhere-data/5.1.9/httpse.sqlite";
    public static final String HTTPS_URL_NEW = "https://s3.amazonaws.com/https-everywhere-data/5.2/httpse.leveldb.zip";
    public static final String HTTPS_LOCALFILENAME_NEW = "httpse.leveldb.zip";
    public static final String HTTPS_LEVELDB_FOLDER = "httpse.leveldb";
    public static final String HTTPS_LOCALFILENAME_DOWNLOADED_NEW = "httpse.leveldbDownloaded.zip";
    public static final String HTTPS_LOCALFILENAME = "httpse.sqlite";
    public static final String HTTPS_LOCALFILENAME_DOWNLOADED = "httpseDownloaded.sqlite";
    public static final String ETAG_PREPEND_HTTPS = "rs";

    public static final long MILLISECONDS_IN_A_DAY = 86400 * 1000;
    public static final int BUFFER_TO_READ = 16384;    // 16Kb

    private static final String ETAGS_PREFS_NAME = "EtagsPrefsFile";
    private static final String ETAG_NAME = "Etag";
    private static final String TIME_NAME = "Time";

    public static void saveETagInfo(Context context, String prepend, EtagObject etagObject) {
        SharedPreferences sharedPref = context.getSharedPreferences(ETAGS_PREFS_NAME, 0);

        SharedPreferences.Editor editor = sharedPref.edit();

        editor.putString(prepend + ETAG_NAME, etagObject.mEtag);
        editor.putLong(prepend + TIME_NAME, etagObject.mMilliSeconds);

        editor.apply();
    }

    public static EtagObject getETagInfo(Context context, String prepend) {
        SharedPreferences sharedPref = context.getSharedPreferences(ETAGS_PREFS_NAME, 0);

        EtagObject etagObject = new EtagObject();

        etagObject.mEtag = sharedPref.getString(prepend + ETAG_NAME, "");
        etagObject.mMilliSeconds = sharedPref.getLong(prepend + TIME_NAME, 0);

        return etagObject;
    }

    public static String getDataVerNumber(String url) {
        String[] split = url.split("/");
        if (split.length > 2) {
            return split[split.length - 2];
        }

        return "";
    }

    public static void removeOldVersionFiles(Context context, String fileName) {
        File dataDirPath = new File(PathUtils.getDataDirectory());
        if (null == dataDirPath) {
            return;
        }
        File[] fileList = dataDirPath.listFiles();

        for (File file : fileList) {
            String sFileName = file.getAbsoluteFile().toString();
            if (sFileName.endsWith(fileName) || sFileName.endsWith(fileName + ".tmp")) {
                file.delete();
            } else if (file.isDirectory() && sFileName.endsWith(ADBlockUtils.HTTPS_LEVELDB_FOLDER)) {
                File[] httpsFileList = file.listFiles();
                for (File httpsFile : httpsFileList) {
                    httpsFile.delete();
                }
                file.delete();
            }
        }
    }

    public static byte[] readLocalFile(File path) {
        byte[] buffer = null;

        FileInputStream inputStream = null;
        try {
            if (!path.exists()) {
                return null;
            }
            inputStream = new FileInputStream(path.getAbsolutePath());
            int size = inputStream.available();
            buffer = new byte[size];
            int n = - 1;
            int bytesOffset = 0;
            byte[] tempBuffer = new byte[ADBlockUtils.BUFFER_TO_READ];
            while ( (n = inputStream.read(tempBuffer)) != -1) {
                System.arraycopy(tempBuffer, 0, buffer, bytesOffset, n);
                bytesOffset += n;
            }
            inputStream.close();
        } catch (IOException e) {
            e.printStackTrace();
        }

        return buffer;
    }

    public static byte[] readData(Context context, String fileName, String urlString, String eTagPrepend, String verNumber,
            String fileNameDownloaded, boolean downloadOnly) {
        File dataPath = new File(PathUtils.getDataDirectory(), verNumber + fileName);
        long oldFileSize = dataPath.length();
        EtagObject previousEtag = ADBlockUtils.getETagInfo(context, eTagPrepend);
        long milliSeconds = Calendar.getInstance().getTimeInMillis();
        if (0 == oldFileSize || (milliSeconds - previousEtag.mMilliSeconds >= ADBlockUtils.MILLISECONDS_IN_A_DAY)) {
            File dataPathCreated = new File(
                PathUtils.getDataDirectory(),
                fileNameDownloaded);
            if (null != dataPathCreated && dataPathCreated.exists()) {
                try {
                    dataPathCreated.delete();
                }
                catch (SecurityException exc) {
                }
            }
            ADBlockUtils.downloadDatFile(context, oldFileSize, previousEtag, milliSeconds, fileName, urlString, eTagPrepend, verNumber);
        }

        if (downloadOnly) {
            return null;
        }

        return readLocalFile(dataPath);
    }

    public static void downloadDatFile(Context context, long oldFileSize, EtagObject previousEtag, long currentMilliSeconds,
                                       String fileName, String urlString, String eTagPrepend, String verNumber) {
        byte[] buffer = null;
        InputStream inputStream = null;
        HttpURLConnection connection = null;

        try {
          Log.i("ADB", "Downloading %s", verNumber + fileName);
            URL url = new URL(urlString);
            connection = (HttpURLConnection) url.openConnection();
            String etag = connection.getHeaderField("ETag");
            int length = connection.getContentLength();
            if (null == etag) {
                etag = "";
            }
            boolean downloadFile = true;
            if (oldFileSize == length && etag.equals(previousEtag.mEtag)) {
                downloadFile = false;
            }
            previousEtag.mEtag = etag;
            previousEtag.mMilliSeconds = currentMilliSeconds;
            ADBlockUtils.saveETagInfo(context, eTagPrepend, previousEtag);
            if (!downloadFile) {
                return;
            }
            ADBlockUtils.removeOldVersionFiles(context, fileName);

            connection.connect();

            if (connection.getResponseCode() != HttpURLConnection.HTTP_OK) {
                return;
            }

            // Write to .tmp file and rename it to dat if success
            File path = new File(PathUtils.getDataDirectory(), verNumber + fileName + ".tmp");
            FileOutputStream outputStream = new FileOutputStream(path);
            inputStream = connection.getInputStream();
            buffer = new byte[ADBlockUtils.BUFFER_TO_READ];
            int n = - 1;
            int totalReadSize = 0;
            try {
                while ((n = inputStream.read(buffer)) != -1) {
                    outputStream.write(buffer, 0, n);
                    totalReadSize += n;
                }
            }
            catch (IllegalStateException exc) {
                // Sometimes it gives us that exception, found that we should do that way to avoid it:
                // Each HttpURLConnection instance is used to make a single request but the
                // underlying network connection to the HTTP server may be transparently shared by other instance.
                // But we do that way, so just wrapped it for now and we will redownload the file on next request
            }
            outputStream.close();
            if (length != totalReadSize || length != path.length()) {
                ADBlockUtils.removeOldVersionFiles(context, fileName);
            } else {
              // We downloaded the file with success, rename it now to .dat
              File renameTo = new File(PathUtils.getDataDirectory(), verNumber + fileName);
              if (!path.exists() || !path.renameTo(renameTo)) {
                  ADBlockUtils.removeOldVersionFiles(context, fileName);
              }
              Log.i("ADB", "Downloaded %s", verNumber + fileName);
            }
        }
        catch (MalformedURLException e) {
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        finally {
            try {
                if (inputStream != null) {
                    inputStream.close();
                }
            }
            catch (IOException e) {
                e.printStackTrace();
            }

            if (connection != null)
                connection.disconnect();
        }

        return;
    }

    public static void CreateDownloadedFile(Context context, String fileName,
                                            String verNumber, String fileNameDownloaded) {
        try {
          Log.i("ADB", "Creating %s", fileNameDownloaded);
            File dataPath = new File(PathUtils.getDataDirectory(), verNumber + fileName);
            if (null != dataPath && (0 != dataPath.length() || dataPath.isDirectory())) {
               File dataPathCreated = new File(PathUtils.getDataDirectory(), fileNameDownloaded);
               if (null != dataPathCreated && !dataPathCreated.exists()) {
                   dataPathCreated.createNewFile();
                   if (dataPathCreated.exists()) {
                       FileOutputStream fo = new FileOutputStream(dataPathCreated);
                       fo.write((verNumber + fileName).getBytes());
                       fo.close();
                       Log.i("ADB", "Created %s", fileNameDownloaded);
                   }
               }
            }
        }
        catch (NullPointerException exc) {
            // We will try to download the file again on next start
        }
        catch (IOException exc) {
        }
    }

    public static boolean UnzipFile(String zipName, String verNumber, boolean removeZipFile) {
        ZipInputStream zis = null;
        try {
            String dir = PathUtils.getDataDirectory();
            File zipFullName =  new File(dir, verNumber + zipName);
            if (null == zipFullName) {
                return false;
            }
            zis = new ZipInputStream(new FileInputStream(zipFullName));
            if (null == zis) {
                Log.i("ADB", "Open zip file " + verNumber + zipName + " error");

                return false;
            }
            byte[] buffer = new byte[ADBlockUtils.BUFFER_TO_READ];
            if (null == buffer) {
                zis.close();

                return false;
            }
            ZipEntry ze = zis.getNextEntry();
            int readBytes = 0;
            String fileName;

            while (null != ze) {
                fileName = ze.getName();
                File fmd = new File(dir, verNumber + fileName);
                if (null == fmd) {
                    zis.closeEntry();
                    zis.close();

                    return false;
                }
                if (ze.isDirectory()) {
                    fmd.mkdirs();
                } else {
                    FileOutputStream fout = new FileOutputStream(fmd);
                    if (null == fout) {
                        zis.closeEntry();
                        zis.close();

                        return false;
                    }

                    int total = 0;
                    while ((readBytes = zis.read(buffer)) != -1) {
                        fout.write(buffer, 0, readBytes);
                        total += readBytes;
                    }
                    fout.close();
                }

                zis.closeEntry();
                ze = zis.getNextEntry();
            }

            zis.close();
            if (removeZipFile) {
                zipFullName.delete();
            }
        } catch (NullPointerException exc) {
            try {
                if (null != zis) {
                    zis.close();
                }
            } catch (IOException ex) {
                return false;
            }

            return false;
        } catch (IOException exc) {
            return false;
        }

        return true;
    }
}
