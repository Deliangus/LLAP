package cn.sencs.llap;

public class Key {

    public int sound;
    public boolean down;
    public int start;
    public int end;

    public Key(int sound, int start, int end) {
        this.sound = sound;
        this.start = start;
        this.end = end;
    }
}