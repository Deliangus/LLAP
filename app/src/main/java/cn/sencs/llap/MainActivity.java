package cn.sencs.llap;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.io.OutputStream;
import java.net.Socket;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("LLAP");
    }

    private Button btnPlayRecord;
    private Button btnStopRecord;
    private TextView texDistance_x;
    private TextView texDistance_y;
    private TraceView mytrace;
    private int recBufSize = 0;
    private int frameSize = 512;
    private double disx, disy;
    private double displaydis = 0;
    private String sysname = "llap";
    private AudioRecord audioRecord;
    private double temperature = 20;
    private double freqinter = 350;
    private int numfreq = 16;
    private double[] wavefreqs = new double[numfreq];
    private double[] wavelength = new double[numfreq];
    private double[] phasechange = new double[numfreq * 2];
    private double[] freqpower = new double[numfreq * 2];
    private double[] dischange = new double[2];
    private double[] idftdis = new double[2];
    private double startfreq = 15050;//17150
    private double soundspeed = 0;
    private int playBufSize;
    private boolean sendDatatoMatlab = false;
    private boolean sendbaseband = false;
    private boolean logenabled = false;
    /**
     *
     */
    private boolean blnPlayRecord = false;
    private int coscycle = 1920;
    /**
     *
     */
    //private int sampleRateInHz = 44100;
    private int sampleRateInHz = 48000;
    /**
     *
     */
    //private int channelConfig = AudioFormat.CHANNEL_CONFIGURATION_MONO;
    private int channelConfig = AudioFormat.CHANNEL_IN_STEREO;
    /**
     *
     */
    private int encodingBitrate = AudioFormat.ENCODING_PCM_16BIT;
    private int cicdec = 16;
    //private int cicsec = 3;
    //private int cicdelay = cicdec * 17;
    private double[] baseband = new double[2 * numfreq * 2 * frameSize / cicdec];
    private double[] baseband_nodc = new double[2 * numfreq * 2 * frameSize / cicdec];
    private short[] dcvalue = new short[4 * numfreq];
    private int[] trace_x = new int[1000];
    private int[] trace_y = new int[1000];
    private int[] tracecount = new int[1];
    private boolean isCalibrated = false;
    private int now;
    private int lastcalibration;
    private double distrend = 0.05;
    private double micdis1 = 5;
    private double micdis2 = 115;
    private double dischangehist = 0;
    /**
     *
     */


    private Socket datasocket;
    private OutputStream datastream;
    private MessageHandler updateviews;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        //FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        //fab.setOnClickListener(new View.OnClickListener() {
        //    @Override
        //    public void onClick(View view) {
        //        Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
        //                .setAction("Action", null).show();
        //    }
        //});

        btnPlayRecord = (Button) findViewById(R.id.btnplayrecord);
        btnStopRecord = (Button) findViewById(R.id.btnstoprecord);
        texDistance_x = (TextView) findViewById(R.id.textView);
        texDistance_y = (TextView) findViewById(R.id.texdistance);
        mytrace = (TraceView) findViewById(R.id.trace);


        soundspeed = 331.3 + 0.606 * temperature;


        for (int i = 0; i < numfreq; i++) {
            wavefreqs[i] = startfreq + i * freqinter;
            wavelength[i] = soundspeed / wavefreqs[i] * 1000;
        }


        disx = 0;
        disy = 250;
        now = 0;
        lastcalibration = 0;

        updateviews = new MessageHandler(texDistance_x, texDistance_y, mytrace, trace_x, trace_y, tracecount);


        mylog("initialization start at time: " + System.currentTimeMillis());
        mylog(initdownconvert(sampleRateInHz, numfreq, wavefreqs));

        mylog("initialization finished at time: " + System.currentTimeMillis());


        //
        btnPlayRecord.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                btnPlayRecord.setEnabled(false);
                btnStopRecord.setEnabled(true);

                recBufSize = AudioRecord.getMinBufferSize(sampleRateInHz,
                        channelConfig, encodingBitrate);

                mylog("recbuffersize:" + recBufSize);

                playBufSize = AudioTrack.getMinBufferSize(sampleRateInHz,
                        channelConfig, encodingBitrate);

                audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC,
                        sampleRateInHz, channelConfig, encodingBitrate, recBufSize);


                mylog("channels:" + audioRecord.getChannelConfiguration());

                new ThreadInstantPlay().start();
                new ThreadInstantRecord().start();
                new ThreadSocket().start();
            }
        });
        //
        btnStopRecord.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                btnPlayRecord.setEnabled(true);
                btnStopRecord.setEnabled(false);
                blnPlayRecord = false;
                isCalibrated = false;
                try {
                    datastream.close();
                    datasocket.close();
                } catch (Exception e) {
                    //TODOL handle this
                }

            }
        });

    }

    private void mylog(String information) {
        if (logenabled) {

            Log.i(sysname, information);
        }
    }

    public native String getbaseband(short[] data, double[] outdata, int numdata);

    public native String removedc(double[] data, double[] data_nodc, short[] outdata);

    // C implementation of down converter

    public native String getdistance(double[] data, double[] outdata, double[] distance, double[] freqpower);

    //C implementation of LEVD

    public native String initdownconvert(int samplerate, int numfreq, double[] wavfreqs);

    //C implementation of distance

    public native String getidftdistance(double[] data, double[] outdata);

    // Initialize C down converter

    public native String calibrate(double[] data);

    class ThreadSocket extends Thread {
        public void run() {
            try {
                datasocket = new Socket("192.168.1.10", 12345);//"114.212.85.187"
                mylog("socket connected" + datasocket);
                datastream = datasocket.getOutputStream();
                mylog("socketsream:" + datastream);
                //datastream.write("test".getBytes());

            } catch (Exception e) {
                // TODO: handle this
                mylog("socket error" + e);
            }
        }
    }

    class ThreadInstantPlay extends Thread {
        @Override
        public void run() {
            SoundPlayer Player = new SoundPlayer(sampleRateInHz, numfreq, wavefreqs);
            blnPlayRecord = true;
            Player.play();
            while (blnPlayRecord == true) {
            }
            Player.stop();
        }
    }

    class ThreadInstantRecord extends Thread {

        //private short [] bsRecord = new short[recBufSize];
        //

        @Override
        public void run() {
            short[] bsRecord = new short[recBufSize * 2];
            byte[] networkbuf = new byte[recBufSize * 4];
            int datacount = 0;
            int curpos = 0;
            long starttime, endtime;
            String c_result;

            while (blnPlayRecord == false) {
            }
            audioRecord.startRecording();
            /*
             *
             */
            while (blnPlayRecord) {
                /*
                 *
                 */
                int line = audioRecord.read(bsRecord, 0, frameSize * 2);
                datacount = datacount + line / 2;
                now = now + 1;

                mylog("recevied data:" + line + " at time" + System.currentTimeMillis());
                if (line >= frameSize) {

                    //get baseband


                    starttime = System.currentTimeMillis();
                    mylog(getbaseband(bsRecord, baseband, line / 2));
                    endtime = System.currentTimeMillis();

                    mylog("time used forbaseband:" + (endtime - starttime));

                    starttime = System.currentTimeMillis();
                    mylog(removedc(baseband, baseband_nodc, dcvalue));
                    endtime = System.currentTimeMillis();

                    mylog("time used LEVD:" + (endtime - starttime));

                    starttime = System.currentTimeMillis();
                    mylog(getdistance(baseband_nodc, phasechange, dischange, freqpower));
                    endtime = System.currentTimeMillis();

                    mylog("time used distance:" + (endtime - starttime));


                    if (!isCalibrated && Math.abs(dischange[0]) < 0.05 && now - lastcalibration > 10) {


                        c_result = calibrate(baseband);
                        mylog(c_result);
                        lastcalibration = now;
                        if (c_result.equals("calibrate OK")) {
                            isCalibrated = true;
                        }

                    }
                    if (isCalibrated) {
                        starttime = System.currentTimeMillis();
                        mylog(getidftdistance(baseband_nodc, idftdis));
                        endtime = System.currentTimeMillis();

                        mylog("time used idftdistance:" + (endtime - starttime));

                        //keep difference stable;

                        double disdiff, dissum;
                        disdiff = dischange[0] - dischange[1];
                        dissum = dischange[0] + dischange[1];
                        dischangehist = dischangehist * 0.5 + disdiff * 0.5;
                        dischange[0] = (dissum + dischangehist) / 2;
                        dischange[1] = (dissum - dischangehist) / 2;

                        disx = disx + dischange[0];
                        if (disx > 1000)
                            disx = 1000;
                        if (disx < 0)
                            disx = 0;
                        disy = disy + dischange[1];
                        if (disy > 1000)
                            disy = 1000;
                        if (disy < 0)
                            disy = 0;
                        if (Math.abs(dischange[0]) < 0.05 && Math.abs(dischange[1]) < 0.05 && Math.abs(idftdis[0]) > 0.1 && Math.abs(idftdis[1]) > 0.1) {
                            disx = disx * (1 - distrend) + idftdis[0] * distrend;
                            disy = disy * (1 - distrend) + idftdis[1] * distrend;
                        }
                        if (disx < micdis1)
                            disx = micdis1;
                        if (disy < micdis2)
                            disy = micdis2;
                        if (Math.abs(disx - disy) > (micdis1 + micdis2)) {
                            double tempsum = disx + disy;
                            if (disx > disy) {
                                disx = (tempsum + micdis1 + micdis2) / 2;
                                disy = (tempsum - micdis1 - micdis2) / 2;

                            } else {
                                disx = (tempsum - micdis1 - micdis2) / 2;
                                disy = (tempsum + micdis1 + micdis2) / 2;
                            }
                        }
                        trace_x[tracecount[0]] = (int) Math.round((disy * micdis1 * micdis1 - disx * micdis2 * micdis2 + disx * disy * (disy - disx)) / 2 / (disx * micdis2 + disy * micdis1));
                        trace_y[tracecount[0]] = (int) Math.round(Math.sqrt(Math.abs((disx * disx - micdis1 * micdis1) * (disy * disy - micdis2 * micdis2) * ((micdis1 + micdis2) * (micdis1 + micdis2) - (disx - disy) * (disx - disy)))) / 2 / (disx * micdis2 + disy * micdis1));
                        mylog("x=" + trace_x[tracecount[0]] + "y=" + trace_y[tracecount[0]]);
                        tracecount[0]++;

                    }


                    if (Math.abs(displaydis - disx) > 2 || (tracecount[0] > 10)) {
                        Message msg = new Message();
                        msg.what = MessageHandler.UPDATEDISTANCE;
                        msg.arg1 = MessageHandler.DistanceUpdateX;
                        msg.arg2 = (int) disx;
                        updateviews.sendMessage(msg);
                        msg = new Message();
                        msg.what = MessageHandler.UPDATEDISTANCE;
                        msg.arg1 = MessageHandler.DistanceUpdateY;
                        msg.arg2 = (int) disy;
                        updateviews.sendMessage(msg);
                    }
                    if (!isCalibrated) {
                        Message msg = new Message();
                        msg.what = MessageHandler.UNCALIBRATED;
                        updateviews.sendMessage(msg);
                    }


                    curpos = curpos + line / 2;
                    if (curpos > coscycle)
                        curpos = curpos - coscycle;
                    if (sendbaseband && datastream != null) {
                        int j = 0;
                        for (int i = 0; i < 2 * numfreq * 2 * frameSize / cicdec; i++) {
                            //sum = sum + bsRecord[i];
                            networkbuf[j++] = (byte) (((short) baseband_nodc[i]) & 0xFF);
                            networkbuf[j++] = (byte) (((short) baseband_nodc[i]) >> 8);
                        }
                        //Log.i("wavedemo", "data sum:" + sum);

                        if (datastream != null) {
                            try {
                                datastream.write(networkbuf, 0, j);
                                mylog("socket write" + j);
                            } catch (Exception e) {
                                // TODO: handle this
                                mylog("socket error" + e);
                            }
                        }

                    }

                    if (sendDatatoMatlab && datastream != null) {
                        int j = 0;
                        int sum = 0;
                        for (int i = 0; i < line; i++) {
                            //sum = sum + bsRecord[i];
                            networkbuf[j++] = (byte) (bsRecord[i] & 0xFF);
                            networkbuf[j++] = (byte) (bsRecord[i] >> 8);
                        }
                        //Log.i("wavedemo", "data sum:" + sum);

                        if (datastream != null) {
                            try {
                                datastream.write(networkbuf, 0, j);
                                mylog("socket write" + j);
                            } catch (Exception e) {
                                // TODO: handle this
                                mylog("socket error" + e);
                            }
                        }
                    }
                }
                mylog("endtime" + System.currentTimeMillis());

            }
            audioRecord.stop();

        }
    }
}
