/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.example.nativeaudio;

import android.Manifest;
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.Toast;

public class NativeAudio extends Activity
        implements ActivityCompat.OnRequestPermissionsResultCallback {

    static final String TAG = "AudioManagerTest";
    private static final int AUDIO_ECHO_REQUEST = 0;

    AssetManager assetManager;

    static int play_state = 0;  // 0->stopped 1->start 2->paused
    static int asset_play_state = 0;  // 0->stopped 1->start 2->paused
    static int record_state = 0;  // 0->stopped 1->start

    /** Called when the activity is first created. */
    @Override
    @TargetApi(17)
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        setContentView(R.layout.main);
        assetManager = getAssets();

        // initialize button click handlers
        ((Button) findViewById(R.id.start_echo)).setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                int status = ActivityCompat.checkSelfPermission(NativeAudio.this,
                        Manifest.permission.RECORD_AUDIO);
                if (status != PackageManager.PERMISSION_GRANTED) {
                    ActivityCompat.requestPermissions(
                            NativeAudio.this,
                            new String[]{Manifest.permission.RECORD_AUDIO},
                            AUDIO_ECHO_REQUEST);
                    //return;
                }
                // ignore the return value
                if (0 == play_state) {
                    play_state = 1;
                    openEcho();
                    startEcho();
                } else if (2 == play_state) {
                    play_state = 1;
                    startEcho();
                }
            }
        });
        ((Button) findViewById(R.id.pause_echo)).setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                // ignore the return value
                if (2 == play_state) {
                    play_state = 1;
                    startEcho();
                } else if (1 == play_state) {
                    play_state = 2;
                    pauseEcho();
                }
            }
        });
        ((Button) findViewById(R.id.stop_echo)).setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                // ignore the return value
                if (1 == play_state || 2 == play_state) {
                    play_state = 0;
                    stopEcho();
                    closeEcho();
                }
            }
        });
        ((Button) findViewById(R.id.echo_vol_up)).setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                echoVolUp();
            }
        });
        ((Button) findViewById(R.id.echo_vol_down)).setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                echoVolDown();
            }
        });
        ((Button) findViewById(R.id.echo_player_mute)).setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                echoMute();
            }
        });

        ((Button) findViewById(R.id.start_uri)).setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                // ignore the return value
                if (0 == play_state) {
                    play_state = 1;
                    openURI();
                    startURI();
                } else if (2 == play_state) {
                    play_state = 1;
                    startURI();
                }
            }
        });
        ((Button) findViewById(R.id.pause_uri)).setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                // ignore the return value
                if (2 == play_state) {
                    play_state = 1;
                    startURI();
                } else if (1 == play_state) {
                    play_state = 2;
                    pauseURI();
                }
            }
        });
        ((Button) findViewById(R.id.stop_uri)).setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                // ignore the return value
                if (1 == play_state || 2 == play_state) {
                    play_state = 0;
                    stopURI();
                    closeURI();
                }
            }
        });

        ((Button) findViewById(R.id.start_assets)).setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                // ignore the return value
                if (0 == asset_play_state) {
                    asset_play_state = 1;
                    openASSETS(assetManager, "mydream.m4a");
                    startASSETS();
                } else if (2 == asset_play_state) {
                    asset_play_state = 1;
                    startASSETS();
                }
            }
        });
        ((Button) findViewById(R.id.pause_assets)).setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                // ignore the return value
                if (2 == asset_play_state) {
                    asset_play_state = 1;
                    startASSETS();
                } else if (1 == asset_play_state) {
                    asset_play_state = 2;
                    pauseASSETS();
                }
            }
        });
        ((Button) findViewById(R.id.stop_assets)).setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                // ignore the return value
                if (1 == asset_play_state || 2 == asset_play_state) {
                    asset_play_state = 0;
                    stopASSETS();
                    closeASSETS();
                }
            }
        });
        ((Button) findViewById(R.id.asset_vol_up)).setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                assetVolUp();

            }
        });
        ((Button) findViewById(R.id.asset_vol_down)).setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                assetVolDown();

            }
        });
        ((Button) findViewById(R.id.asset_player_mute)).setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                assetMute();

            }
        });

        ((Button) findViewById(R.id.start_record)).setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                int status = ActivityCompat.checkSelfPermission(NativeAudio.this,
                        Manifest.permission.RECORD_AUDIO);
                if (status != PackageManager.PERMISSION_GRANTED) {
                    ActivityCompat.requestPermissions(
                            NativeAudio.this,
                            new String[]{Manifest.permission.RECORD_AUDIO},
                            AUDIO_ECHO_REQUEST);
                    //return;
                }
                // ignore the return value
                if (0 == record_state) {
                    record_state = 1;
                    openRecorder();
                    recording();
                }
            }
        });
        ((Button) findViewById(R.id.stop_record)).setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                // ignore the return value
                if (1 == record_state) {
                    record_state = 0;
                    stopRecorder();
                    closeRecorder();
                }
            }
        });
        ((Button) findViewById(R.id.get_version)).setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                // ignore the return value
                getVersion();
            }
        });
    }


   /** Called when the activity is about to be destroyed. */
    @Override
    protected void onPause()
    {
        // turn off all audio
        super.onPause();
    }

    /** Called when the activity is about to be destroyed. */
    @Override
    protected void onDestroy()
    {
        super.onDestroy();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        /*
         * if any permission failed, the sample could not play
         */
        if (AUDIO_ECHO_REQUEST != requestCode) {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);
            return;
        }

        if (grantResults.length != 1  ||
                grantResults[0] != PackageManager.PERMISSION_GRANTED) {
            /*
             * When user denied the permission, throw a Toast to prompt that RECORD_AUDIO
             * is necessary; on UI, we display the current status as permission was denied so
             * user know what is going on.
             * This application go back to the original state: it behaves as if the button
             * was not clicked. The assumption is that user will re-click the "start" button
             * (to retry), or shutdown the app in normal way.
             */
            Toast.makeText(getApplicationContext(),
                    getString(R.string.NeedRecordAudioPermission),
                    Toast.LENGTH_SHORT)
                    .show();
            return;
        }

        // The callback runs on app's thread, so we are safe to resume the action
        //recordAudio();
    }

    /** Native methods, implemented in jni folder */
    public static native void openEcho();
    public static native void startEcho();
    public static native void pauseEcho();
    public static native void stopEcho();
    public static native void closeEcho();
    public static native void echoVolUp();
    public static native void echoVolDown();
    public static native void echoMute();

    public static native void startURI();
    public static native void openURI();
    public static native void pauseURI();
    public static native void stopURI();
    public static native void closeURI();

    public static native void openASSETS(AssetManager assetManager, String filename);
    public static native void startASSETS();
    public static native void pauseASSETS();
    public static native void stopASSETS();
    public static native void closeASSETS();
    public static native void assetVolUp();
    public static native void assetVolDown();
    public static native void assetMute();

    public static native void openRecorder();
    public static native void recording();
    public static native void stopRecorder();
    public static native void closeRecorder();

    public static native void getVersion();
    /** Load jni .so on initialization */
    static {
         System.loadLibrary("native-audio-jni");
    }

}
