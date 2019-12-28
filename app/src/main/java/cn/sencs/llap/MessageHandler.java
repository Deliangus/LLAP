package cn.sencs.llap;

import android.media.MediaRecorder;
import android.os.Handler;

public class MessageHandler extends Handler {
    public static int UPDATEDISTANCE = 1;
    public static int AUDIOSOURCE_MIC = MediaRecorder.AudioSource.MIC;
    public static int AUDIOSOURCE_CAM = MediaRecorder.AudioSource.CAMCORDER;

    public MessageHandler() {
        super();
    }

}
