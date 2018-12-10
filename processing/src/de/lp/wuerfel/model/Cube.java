package de.lp.wuerfel.model;

public class Cube extends BaseCube {

    private long lastUpdateMillis;

    private int uid;
    private String name;
    private boolean selected;

    public Cube() {
    }

    public Cube(int uid, String name) {
        this.uid = uid;
        this.name = name;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public int getUid() {
        return uid;
    }

    public void setUid(int uid) {
        this.uid = uid;
    }

    public long getLastUpdateMillis() {
        return lastUpdateMillis;
    }

    public void setLastUpdateMillis(long lastUpdateMillis) {
        this.lastUpdateMillis = lastUpdateMillis;
    }

    public boolean isSelected() {
        return selected;
    }

    public void setSelected(boolean selected) {
        this.selected = selected;
    }
}
