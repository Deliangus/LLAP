package cn.sencs.llap;


import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;
import android.util.SparseArray;

import java.io.InputStream;

public class AudioSoundPlayer {

    public static final int MAX_VOLUME = 100, CURRENT_VOLUME = 90;
    private static final SparseArray<String> SOUND_MAP = new SparseArray<>();

    static {
        // white keys sounds
        SOUND_MAP.put(1, "do");
        SOUND_MAP.put(2, "re");
        SOUND_MAP.put(3, "mi");
        SOUND_MAP.put(4, "fa");
        SOUND_MAP.put(5, "sol");
        SOUND_MAP.put(6, "la");
        SOUND_MAP.put(7, "si");
        SOUND_MAP.put(8, "do-oct");
    }

    private final String TAG = "AudioSoundPlayer:";
    private SparseArray<PlayThread> threadMap = null;
    private Context context;

    public AudioSoundPlayer(Context context) {
        this.context = context;
        threadMap = new SparseArray<>();
    }

    public void playNote(int note) {
        if (!isNotePlaying(note)) {
            PlayThread thread = new PlayThread(note);
            thread.start();
            threadMap.put(note, thread);
        }
    }

    public void stopNote(int note) {
        PlayThread thread = threadMap.get(note);

        if (thread != null) {
            threadMap.remove(note);
        }
    }

    public boolean isNotePlaying(int note) {
        return threadMap.get(note) != null;
    }

    private class PlayThread extends Thread {
        int note;
        AudioTrack audioTrack;

        public PlayThread(int note) {
            this.note = note;
        }

        @Override
        public void run() {
            try {
                Log.d(TAG, SOUND_MAP.get(note) + "was run");
                String path = SOUND_MAP.get(note) + ".wav";
                AssetManager assetManager = context.getAssets();
                AssetFileDescriptor ad = assetManager.openFd(path);
                long fileSize = ad.getLength();
                int bufferSize = 4096;
                byte[] buffer = new byte[bufferSize];

                audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, 44100, AudioFormat.CHANNEL_CONFIGURATION_MONO,
                        AudioFormat.ENCODING_PCM_16BIT, bufferSize, AudioTrack.MODE_STREAM);

                float logVolume = (float) (1 - (Math.log(MAX_VOLUME - CURRENT_VOLUME) / Math.log(MAX_VOLUME)));
                audioTrack.setStereoVolume(logVolume, logVolume);

                audioTrack.play();
                InputStream audioStream = null;
                int headerOffset = 0x2C;
                long bytesWritten = 0;
                int bytesRead = 0;

                audioStream = assetManager.open(path);
                audioStream.read(buffer, 0, headerOffset);

                while (bytesWritten < fileSize - headerOffset) {
                    bytesRead = audioStream.read(buffer, 0, bufferSize);
                    bytesWritten += audioTrack.write(buffer, 0, bytesRead);
                }
                audioTrack.stop();
                audioTrack.release();

            } catch (Exception e) {
                Log.d(TAG, e.toString());
            } finally {
                if (audioTrack != null) {
                    audioTrack.release();
                }
            }
        }
    }

}