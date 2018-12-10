package de.lp.wuerfel;

import controlP5.*;
import de.lp.wuerfel.model.BaseCube;
import de.lp.wuerfel.model.Cube;
import de.lp.wuerfel.model.PresetItem;
import processing.core.PApplet;
import processing.core.PFont;
import processing.serial.Serial;

import java.util.*;

import static controlP5.ControlP5Constants.ACTION_RELEASE;


public class MainWindow extends PApplet {

    private static int REFRESH_TIMEOUT = 1000;
    private static int MAX_COLOR_HISTORY = 12;

    private boolean blackout = false;
    private int wheelcolor = color(50);

    private Serial gateway;
    private ScrollableList presetList;
    private Textfield presetNameField;
    private ControlP5 cp5;
    private MyMatrix matrix;
    private boolean sendEnable = false;
    private boolean doRefreshing = true;

    private List<Bang> lastColors = new ArrayList<>();


    public void settings() {
        size(1000, 900);
    }

    public void setup() {
        frameRate(30);
        colorMode(RGB, 255);

        gateway = new Serial(this, "/dev/ttyUSB0", 115200);
        cp5 = new ControlP5(this);

        cp5.addSlider("overallbrightness").setPosition(10, 10).setSize(200, 40).setRange(0, 1.0f).setValue(0.5f);
        cp5.addSlider("fadetime").setPosition(10, 50).setSize(200, 40).setRange(0, 255).setValue(0);


        cp5.addToggle("setBlackout").setPosition(320, 10).setSize(40, 40).setValue(false).setLabel("Blackout");
        cp5.addToggle("sendEnable").setPosition(380, 10).setSize(40, 40).setValue(false).setLabel("Enable");


        presetList = cp5.addScrollableList("presetlist").setPosition(10, 280).setSize(200, 500)
                .setBarHeight(30).setItemHeight(30).setType(ScrollableList.LIST).setBarVisible(true);


        matrix = new MyMatrix(cp5, "myMatrix", 5, 10, 220, 280, 200, 400);
        cp5.addBang("clearSelection").setPosition(220, 230).setSize(30, 30).setLabel("Clear").addListener(controlEvent -> matrix.clear());
        cp5.addBang("oddSelection").setPosition(260, 230).setSize(30, 30).setLabel("Odd").addListener(controlEvent -> matrix.odd());
        cp5.addBang("evenSelection").setPosition(300, 230).setSize(30, 30).setLabel("Even").addListener(controlEvent -> matrix.even());
        cp5.addBang("allSelection").setPosition(340, 230).setSize(30, 30).setLabel("All").addListener(controlEvent -> matrix.all());

        for (int i = 0; i < MAX_COLOR_HISTORY; i++) {
            lastColors.add(cp5.addBang("colorX" + i).setPosition(468 + i * 42, 550).setSize(40, 40).setId(i).setLabelVisible(false));
        }

        ColorWheel wheel = cp5.addColorWheel("wheelcolor", 470, 10, 500).setRGB(color(128, 0, 255)).setLabelVisible(false);
        wheel.addCallback(theEvent -> {
            if (theEvent.getAction() == ACTION_RELEASE) {

                for (int i = MAX_COLOR_HISTORY - 1; i > 0; i--) {
                    int col = lastColors.get(i - 1).getColor().getForeground();
                    lastColors.get(i)
                            .setColorActive(col)
                            .setColorForeground(col);
                }

                lastColors.get(0)
                        .setColorForeground((int) theEvent.getController().getValue())
                        .setColorActive((int) theEvent.getController().getValue());

                if (sendEnable) {
                    matrix.getCubeList().stream().filter(Cube::isSelected).forEach(cube -> {
                        writeCube(cube);
                        cube.setLastUpdateMillis(millis());
                    });
                }
            }
        });


        PFont font = createFont("Verdana", 14);
        presetNameField = cp5.addTextfield("presetName").setPosition(10, 790).setSize(120, 25).setFont(font).setFocus(true);

        cp5.addBang("presetupdate").setPosition(10, 830).setSize(40, 30).setLabel("Pre.Updt");
        cp5.addBang("presetnew").setPosition(60, 830).setSize(40, 30).setLabel("Pre.New").setColorForeground(color(0,120,0));
        cp5.addBang("presetdel").setPosition(110, 830).setSize(40, 30).setLabel("Pre.Del").setColorForeground(color(120, 0, 0));

        cp5.addBang("presetprev").setPosition(220, 830).setSize(40, 30).setLabel("Pre.Prev");
        cp5.addBang("presetnext").setPosition(270, 830).setSize(40, 30).setLabel("Pre.Next");


        surface.setTitle("Bjarneswuerfel");
        textSize(11);

        loadPresets();
    }

    public void presetlist(int n) {
        presetLoad(n);
    }

    public void presetupdate() {
        presetUpdate();
        savePresets();
    }

    public void presetnew() {
        presetNew();
        savePresets();
    }

    public void presetdel() {
        if (presetDelete((int) presetList.getValue())) {
            savePresets();
        }
    }

    public void setBlackout(boolean bo) {      // function for the ControlP5 Toggle
        if (this.blackout != bo) {
            gateway.write("A," + (bo ? "1" : "0") + '\n');
        }
        this.blackout = bo;
    }


    private void writeCube(Cube cube) {
        int red = cube.getColor() >> 16 & 0xff;
        int green = cube.getColor() >> 8 & 0xff;
        int blue = cube.getColor() & 0xff;
        gateway.write("B," + cube.getUid() + "," + (cube.getFadetime()) + "," + red + "," + green + "," + blue + '\n');
    }


    public void draw() {
        background(0);

        stroke(255);

        text("#" + hex(wheelcolor, 6), 470, 520);


        // periodically refresh the cubes to make sure all is set as we wanted to
        if (!blackout && doRefreshing) {
            matrix.getCubeList().forEach(cube -> {
                if (millis() - (cube.getLastUpdateMillis() + cube.getFadetime()) > REFRESH_TIMEOUT) {
                    if (sendEnable) {
                        writeCube(cube);
                    }
                    cube.setLastUpdateMillis(millis());
                }
            });
        }
    }

    public void presetprev() {
        int presetInt = (int)presetList.getValue();
        if (presetInt>0) {
            presetList.setValue(presetInt-1);
            presetLoad(presetInt-1);
        }
    }

    public void presetnext() {
        int presetInt = (int)presetList.getValue();
        if (presetInt<presetList.getItems().size()) {
            presetList.setValue(presetInt+1);
            presetLoad(presetInt+1);
        }
    }

    public void keyPressed() {
        if (keyCode == RIGHT) {
            presetnext();
        } else if(keyCode == LEFT) {
            presetprev();
        }
    }

    public void controlEvent(ControlEvent e) {
        if (e.getController().getName().contentEquals("wheelcolor")) {

            matrix.setColorToSelection((int) e.getValue());

        } else if (e.getController().getName().startsWith("colorX")) {

            matrix.setColorToSelection((e.getController().getColor().getForeground()));

            if (sendEnable) {
                matrix.getCubeList().stream().filter(Cube::isSelected).forEach(cube -> {
                    writeCube(cube);
                    cube.setLastUpdateMillis(millis());
                });
            }
        } else if(e.getController().getName().contentEquals("fadetime")) {
            if (matrix != null) {
                matrix.setFadetimeToSelection((int) e.getController().getValue());
            }
        }
    }


    /**
     * update a preset (update name and filenames),  does not update the presets.txt
     */
    private void presetUpdate() {
        int presetIndex = (int) presetList.getValue();

        if (presetIndex < presetList.getItems().size() && presetIndex >= 0) {
            String name = presetNameField.getText();

            if ("".contentEquals(name)) {
                name = "preset " + presetList.getItems().size();
            }

            Map<String, Object> item = presetList.getItem(presetIndex);
            item.put("name", name);
            item.put("text", name);
            PresetItem value = (PresetItem) item.get("value");
            value.setName(name);

            List<BaseCube> cubes = value.getPresetCubes();

            for(int i = 0; i < matrix.getCubeList().size(); i++) {
                BaseCube baseCube = cubes.get(i);
                Cube cube = matrix.getCubeList().get(i);
                baseCube.setColor(cube.getColor());
                baseCube.setFadetime(cube.getFadetime());
            }
        }
    }

    /**
     * save a new preset in the list. does not update the presets.txt
     */
    private void presetNew() {
        String name = presetNameField.getText();

        if ("".contentEquals(name)) {
            name = "preset " + presetList.getItems().size();
        }

        PresetItem preset = new PresetItem(name);
        List<BaseCube> saveCubes = preset.getPresetCubes();

        for (Cube cube : matrix.getCubeList()) {
            BaseCube leCube = new BaseCube();
            leCube.setFadetime(cube.getFadetime());
            leCube.setColor(cube.getColor());
            saveCubes.add(leCube);
        }

        presetList.addItem(preset.getName(), preset);
    }


    /**
     * load a preset. Sets the cubes colors and fadetime, sends the data out instantly
     *
     * @param presetIndex The index in the presetList of the preset
     */
    private void presetLoad(int presetIndex) {
        if (presetIndex < presetList.getItems().size() && presetIndex >= 0) {
            Map<String, Object> item = presetList.getItem(presetIndex);
            if (item != null) {
                PresetItem preset = (PresetItem) item.get("value");
                if (preset != null) {
                    presetNameField.setText(preset.getName());
                    List<Cube> myCubes = matrix.getCubeList();

                    doRefreshing = false;

                    for (int i = 0; i < preset.getPresetCubes().size(); i++) {
                        if (i < myCubes.size()) {
                            Cube leCube = myCubes.get(i);
                            int currentColor = leCube.getColor();
                            BaseCube presetCube = preset.getPresetCubes().get(i);

                            leCube.setColor(presetCube.getColor());
                            leCube.setFadetime(presetCube.getFadetime());

                            if(sendEnable && currentColor != presetCube.getColor()) {
                                writeCube(leCube);
                                leCube.setLastUpdateMillis(millis());
                            }


                        }
                    }

                    doRefreshing = true;
                }
            }
        }
    }

    /**
     * Removes a preset from the presetList without changing the presets.txt file.
     *
     * @param presetIndex The index in the presetList
     * @return false if the preset index is invalid
     */
    private Boolean presetDelete(int presetIndex) {
        if (presetIndex < presetList.getItems().size() && presetIndex >= 0) {
            Map<String, Object> item = presetList.getItem(presetIndex);
            if (item != null) {
                presetList.removeItem(item.get("name").toString());
                return true;
            }
        }
        return false;
    }


    /**
     * save the presets into file
     */
    private void savePresets() {
        List<String> lines = new ArrayList<>();

        for (int i = 0; i < presetList.getItems().size(); i++) {
            Map<String, Object> item = presetList.getItem(i);
            PresetItem presetItem = (PresetItem) item.get("value");
            String line = presetItem.toString();
            lines.add(line);
        }

        saveStrings("presets.txt", lines.toArray(new String[0]));
    }

    /**
     * load presets from file
     */
    private void loadPresets() {
        presetList.clear();
        String[] lines = loadStrings("presets.txt");
        if (lines != null) {
            for (String line : lines) {
                PresetItem presetItem = new PresetItem();
                if (presetItem.load(line)) {
                    presetList.addItem(presetItem.getName(), presetItem);
                }
            }
        }
    }

    /**
     * Sort the presets in the PresetList (does not write the presets.txt)
     */
    private void sortPresets() {
        SortedMap<String, PresetItem> sortedMap = new TreeMap<>();

        for (int i = 0; i < presetList.getItems().size(); i++) {
            Map<String, Object> item = presetList.getItem(i);
            PresetItem presetItem = (PresetItem) item.get("value");
            sortedMap.put(presetItem.getName(), presetItem);
        }

        presetList.clear();

        for (Map.Entry<String, PresetItem> item : sortedMap.entrySet()) {
            presetList.addItem(item.getKey(), item.getValue());
        }
    }


    public static void main(String args[]) {
        PApplet.main("de.lp.wuerfel.MainWindow");
    }
}
