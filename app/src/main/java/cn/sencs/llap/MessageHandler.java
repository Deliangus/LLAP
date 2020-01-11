package cn.sencs.llap;

import android.media.MediaRecorder;
import android.os.Handler;
import android.os.Message;
import android.widget.TextView;

public class MessageHandler extends Handler {
    public static int UPDATEDISTANCE = 1;
    public static int UNCALIBRATED = -1;
    public static int AUDIOSOURCE_MIC = MediaRecorder.AudioSource.MIC;
    public static int AUDIOSOURCE_CAM = MediaRecorder.AudioSource.CAMCORDER;
    public static int DistanceUpdateX = 2;
    public static int DistanceUpdateY = 3;
    private TextView texDistance_x;
    private TextView texDistance_y;
    private TraceView mytrace;
    private int[] trace_x;
    private int[] trace_y;
    private int[] tracecount;

    public MessageHandler(TextView x, TextView y, TraceView tv, int[] tc_x, int[] tc_y, int[] tcc) {
        super();
        texDistance_x = x;
        texDistance_y = y;
        mytrace = tv;
        trace_x = tc_x;
        trace_y = tc_y;
        tracecount = tcc;
    }

    @Override
    public void handleMessage(Message msg) {
        if (msg.what == UPDATEDISTANCE) {


            if (msg.arg1 == DistanceUpdateX) {
                texDistance_x.setText(String.format("x=%04.2f", (float) msg.arg2 / 20) + "cm");
            } else if (msg.arg1 == DistanceUpdateY) {
                texDistance_y.setText(String.format("y=%04.2f", (float) msg.arg2 / 20) + "cm");
            }

            //mylog("count" + tracecount);
            mytrace.setTrace(trace_x, trace_y, tracecount[0]);
            tracecount[0] = 0;
        } else if (msg.what == UNCALIBRATED) {
            texDistance_x.setText("Calibrating...");
            texDistance_y.setText("");
        }
    }
}
