package cn.sencs.llap;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Timer;
import java.util.TimerTask;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("LLAP");
    }

    private final String TAG = "llap";
    public int VOICE_RECOGNITION = getVOICE_RECOGNITION();
    public int VOICE_COMMUNICATION = getVOICE_COMMUNICATION();

    private SeekBar sb_com;
    private TextView tv_com;
    private SeekBar sb_rec;
    private TextView tv_rec;
    private int distance_communication;
    private int distance_recognization;
    private AudioSoundPlayer audioHandler;
    private ArrayList<Key> keys = new ArrayList<>();
    private Timer mTimer = new Timer();
    private ArrayList<Key> waitingQueues = new ArrayList<>();
    private Handler handler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            if (msg.what == 0) {
                tv_com.setText(Integer.toString(distance_communication) + " mm");
                sb_com.setProgress(distance_communication);
                tv_rec.setText(Integer.toString(distance_recognization) + " mm");
                sb_rec.setProgress(distance_recognization);
                /* Judging if this is a pressdown on the key*/
                final Key curKey = keyForDistance(distance_communication);
                if (curKey != null) {
                    Log.d(TAG, "key: " + curKey.sound);
                    TimerTask playSounds = new EnhancedTimerTask(curKey) {
                        @Override
                        public void run() {
                            Key laterKey_com = keyForDistance(distance_communication);
                            Key laterKey_rec = keyForDistance(distance_recognization);
                            if (laterKey_com != null) {
                                if (this.key.sound == laterKey_com.sound) {
                                    audioHandler.playNote(laterKey_com.sound);
                                } else {
                                    for (Key key : keys) {
                                        if (audioHandler.isNotePlaying(key.sound)) {
                                            audioHandler.stopNote(key.sound);
                                        }
                                    }
                                }
                            }

                            if (laterKey_rec != null) {
                                if (this.key.sound == laterKey_rec.sound) {
                                    audioHandler.playNote(laterKey_rec.sound);
                                } else {
                                    for (Key key : keys) {
                                        if (audioHandler.isNotePlaying(key.sound)) {
                                            audioHandler.stopNote(key.sound);
                                        }
                                    }
                                }
                            }
                        }
                    };
                    mTimer.schedule(playSounds, Constants.PAUSING_THRESHOLD);
                }
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        initPermission();
        setContentView(R.layout.activity_main);

        tv_com = findViewById(R.id.textView_com);
        sb_com = findViewById(R.id.seekBar_com);
        tv_rec = findViewById(R.id.textView_rec);
        sb_rec = findViewById(R.id.seekBar_rec);
        audioHandler = new AudioSoundPlayer(getApplicationContext());
        final Button bt = findViewById(R.id.button);
        tv_com.setText("0 mm");
        tv_rec.setText("0 mm");

        /*Hard coding the relation between range and sound playing*/
        keys.add(new Key(1, 0, 12));
        keys.add(new Key(2, 12, 33));
        keys.add(new Key(3, 33, 60));
        keys.add(new Key(4, 60, 90));
        keys.add(new Key(5, 90, 115));
        keys.add(new Key(6, 115, 135));
        keys.add(new Key(7, 135, 200));


        tv_rec.setOnClickListener(new View.OnClickListener() {
            private boolean begined = false;

            @Override
            public void onClick(View view) {
                if (begined) {
                    begined = false;
                    Stop(VOICE_RECOGNITION);
                } else {
                    Begin(VOICE_RECOGNITION);
                    begined = true;
                }
            }
        });
        tv_com.setOnClickListener(new View.OnClickListener() {
            private boolean begined = false;

            @Override
            public void onClick(View view) {
                if (begined) {
                    begined = false;
                    Stop(VOICE_COMMUNICATION);
                } else {
                    Begin(VOICE_COMMUNICATION);
                    begined = true;
                }
            }
        });
    }

    public void refresh(int voice_source, int progress) {
        Message msg = new Message();
        if (voice_source == VOICE_COMMUNICATION) {
            distance_communication = progress;
        } else if (voice_source == VOICE_RECOGNITION) {
            distance_recognization = progress;
        }
        msg.what = 0;
        handler.sendMessage(msg);
    }

    private void initPermission() {
        String permissions[] = {Manifest.permission.RECORD_AUDIO,
                Manifest.permission.ACCESS_NETWORK_STATE,
                Manifest.permission.INTERNET,
                Manifest.permission.READ_PHONE_STATE,
                Manifest.permission.WRITE_EXTERNAL_STORAGE
        };

        ArrayList<String> toApplyList = new ArrayList<String>();

        for (String perm : permissions) {
            if (PackageManager.PERMISSION_GRANTED != ContextCompat.checkSelfPermission(this, perm)) {
                toApplyList.add(perm);
                //进入到这里代表没有权限.

            }
        }
        String tmpList[] = new String[toApplyList.size()];
        if (!toApplyList.isEmpty()) {
            ActivityCompat.requestPermissions(this, toApplyList.toArray(tmpList), 123);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {

    }

    private Key keyForDistance(int distance) {
        for (Key k : keys) {
            if (distance < k.end && distance >= k.start) {
                return k;
            }
        }

        return null;
    }

    public native void Begin(int voice_source);

    public native void Stop(int voice_source);

    public native int getVOICE_RECOGNITION();

    public native int getVOICE_COMMUNICATION();
}
