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
    private boolean begined = false;
    private SeekBar sb;
    private TextView tv;
    private int distance;
    private AudioSoundPlayer audioHandler;
    private ArrayList<Key> keys = new ArrayList<>();
    private Timer mTimer = new Timer();
    private ArrayList<Key> waitingQueues = new ArrayList<>();
    private Handler handler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            if (msg.what == 0) {
                tv.setText(Integer.toString(distance) + " mm");
                sb.setProgress(distance);
                /* Judging if this is a pressdown on the key*/
                final Key curKey = keyForDistance(distance);
                if (curKey != null) {
                    Log.d(TAG, "key: " + curKey.sound);
                    TimerTask playSounds = new EnhancedTimerTask(curKey) {
                        @Override
                        public void run() {
                            Key laterKey = keyForDistance(distance);
                            if (laterKey != null) {
                                if (this.key.sound == laterKey.sound) {
                                    audioHandler.playNote(laterKey.sound);
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

        tv = findViewById(R.id.textView);
        sb = findViewById(R.id.seekBar);
        audioHandler = new AudioSoundPlayer(getApplicationContext());
        final Button bt = findViewById(R.id.button);
        tv.setText("0 mm");

        /*Hard coding the relation between range and sound playing*/
        keys.add(new Key(1, 0, 12));
        keys.add(new Key(2, 12, 33));
        keys.add(new Key(3, 33, 60));
        keys.add(new Key(4, 60, 90));
        keys.add(new Key(5, 90, 115));
        keys.add(new Key(6, 115, 135));
        keys.add(new Key(7, 135, 200));


        bt.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (begined) {
                    begined = false;
                    //TODO: add stop operation
                } else {
                    Begin();
                    begined = true;
                }
            }
        });
    }

    public void refresh(int progress) {
        Message msg = new Message();
        distance = progress;
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
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {

    }

    private Key keyForDistance(int distance) {
        for (Key k : keys) {
            if (distance < k.end && distance >= k.start) {
                return k;
            }
        }

        return null;
    }


    public native void Begin();
}
