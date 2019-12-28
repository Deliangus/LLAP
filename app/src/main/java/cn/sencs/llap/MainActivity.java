package cn.sencs.llap;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
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

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("LLAP");
    }

    private final String TAG = "llap";
    private MessageHandler mHandler;
    private SeekBar sb_mic;
    private TextView tv_mic;
    private SeekBar sb_cam;
    private TextView tv_cam;
    private int distance;
    private AudioSoundPlayer audioHandler;
    private ArrayList<Key> keys = new ArrayList<>();
    private Timer mTimer = new Timer();
    private ArrayList<Key> waitingQueues = new ArrayList<>();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        initPermission();
        setContentView(R.layout.activity_main);

        tv_mic = (TextView) findViewById(R.id.textView_mic);
        sb_mic = (SeekBar) findViewById(R.id.seekBar_mic);
        tv_cam = (TextView) findViewById(R.id.textView_cam);
        sb_cam = (SeekBar) findViewById(R.id.seekBar_cam);

        mHandler = new MessageHandler() {
            @Override
            public void handleMessage(Message msg) {
                Log.e("MessageHandler", String.format("MessageType: %d\tAudioSource: %d\tValue: %d\t", msg.what, msg.arg1, msg.arg2));
                if (msg.what == MessageHandler.UPDATEDISTANCE) {
                    Log.e("MESSAGE", String.format("%d %d %d", msg.what, msg.arg1, msg.arg2));
                    int distance = msg.arg2;
                    if (msg.arg1 == MessageHandler.AUDIOSOURCE_CAM) {
                        //RangeFinder.LogCodeInfo();
                        tv_cam.setText("CAM: " + distance + " mm");
                        sb_cam.setProgress(distance);

                    } else if (msg.arg1 == MessageHandler.AUDIOSOURCE_MIC) {
                        //RangeFinder.LogCodeInfo();
                        tv_mic.setText("MIC: " + distance + " mm");
                        sb_mic.setProgress(distance);
                    }
                }
            }
        };

        audioHandler = new AudioSoundPlayer(getApplicationContext());
        final Button bt = (Button) findViewById(R.id.button);
        tv_mic.setText("0 mm");

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
                Begin();
            }
        });
    }

    public void refresh(int progress, int audioSource) {
        Message msg = new Message();
        distance = progress;
        msg.what = MessageHandler.UPDATEDISTANCE;
        msg.arg1 = audioSource;
        msg.arg2 = progress;
        mHandler.sendMessage(msg);
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
