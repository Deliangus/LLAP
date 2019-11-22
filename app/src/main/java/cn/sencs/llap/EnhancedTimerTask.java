package cn.sencs.llap;

import java.util.TimerTask;

public class EnhancedTimerTask extends TimerTask {

    public final Key key;

    EnhancedTimerTask(Key key){
        this.key = key;
    }

    public void run() {
        //Do stuff
    }
}
