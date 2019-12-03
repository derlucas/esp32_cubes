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
import static java.awt.event.KeyEvent.*;


public class MainWindow extends PApplet {

    private static int REFRESH_TIMEOUT = 500;
    private static int MAX_COLOR_HISTORY = 12;

    private boolean blackout = false;
    private boolean clearAfterColor = false;
    private int wheelcolor = color(50);

    private Serial gateway;
    private ScrollableList presetList;
    private Textfield presetNameField;
    private ControlP5 cp5;
    private MyMatrix matrix;
    private boolean sendEnable = false;
    private boolean doRefreshing = true;

    private List<Bang> lastColors = new ArrayList<>();
    private final int[] presetColors = new int[]{
            color(255), color(0), color(125),
            color(255, 0, 0), color(0, 255, 0), color(0, 0, 255),
            color(255, 255, 0), color(255, 0, 255), color(0, 255, 255),
            color(138, 0, 247), color(40, 111, 0), color(255, 0, 141),
    };


    public void settings() {
        size(1000, 900);
    }

    public void setup() {
        frameRate(30);
        colorMode(RGB, 255);

        String ports[] = Serial.list();
        String port = "";

        for(String p : ports) {
            if(p.contains("ttyUSB0") || p.contains("tty.SLAB_USB")) {
                port = p;
                break;
            }
        }

        gateway = new Serial(this, port, 115200);
        cp5 = new ControlP5(this);

        presetList = cp5.addScrollableList("presetlist").setPosition(10, 10).setSize(200, 750)
                .setBarHeight(0).setItemHeight(30).setType(ScrollableList.LIST).setBarVisible(false);

        cp5.addToggle("setBlackout").setPosition(820, 10).setSize(40, 40).setValue(false).setLabel("Blackout");
        cp5.addToggle("sendEnable").setPosition(880, 10).setSize(40, 40).setValue(false).setLabel("Enable");


        matrix = new MyMatrix(cp5, "myMatrix", 5, 10, 230, 10, 200, 400);
        int y = 230 - 40;

        cp5.addBang("mod3Selection").setPosition(230, 430).setSize(40, 40).setLabel("MOD3 (7)").addListener(new ControlListener() {
            @Override
            public void controlEvent(ControlEvent controlEvent) {
                matrix.selModulo(3);
            }
        });
        cp5.addBang("mod4Selection").setPosition(280, 430).setSize(40, 40).setLabel("MOD4 (8)").addListener(new ControlListener() {
            @Override
            public void controlEvent(ControlEvent controlEvent) {
                matrix.selModulo(4);
            }
        });
        cp5.addBang("mod5Selection").setPosition(330, 430).setSize(40, 40).setLabel("MOD5 (9)").addListener(new ControlListener() {
            @Override
            public void controlEvent(ControlEvent controlEvent) {
                matrix.selModulo(5);
            }
        });
        cp5.addBang("mod6Selection").setPosition(230, 490).setSize(40, 40).setLabel("MOD6 (4)").addListener(new ControlListener() {
            @Override
            public void controlEvent(ControlEvent controlEvent) {
                matrix.selModulo(6);
            }
        });
        cp5.addBang("mod7Selection").setPosition(280, 490).setSize(40, 40).setLabel("MOD7 (5)").addListener(new ControlListener() {
            @Override
            public void controlEvent(ControlEvent controlEvent) {
                matrix.selModulo(7);
            }
        });
        cp5.addBang("oddSelection").setPosition(230, 550).setSize(40, 40).setLabel("Odd (1)").addListener(new ControlListener() {
            @Override
            public void controlEvent(ControlEvent controlEvent) {
                matrix.odd();
            }
        });
        cp5.addBang("evenSelection").setPosition(280, 550).setSize(40, 40).setLabel("Even (2)").addListener(new ControlListener() {
            @Override
            public void controlEvent(ControlEvent controlEvent) {
                matrix.even();
            }
        });
        cp5.addBang("randSelection").setPosition(330, 550).setSize(40, 40).setLabel("rnd (3)").addListener(new ControlListener() {
            @Override
            public void controlEvent(ControlEvent controlEvent) {
                matrix.random();
            }
        });
        cp5.addBang("clearSelection").setPosition(230, 610).setSize(40, 40).setLabel("none (0)").addListener(new ControlListener() {
            @Override
            public void controlEvent(ControlEvent controlEvent) {
                matrix.clear();
            }
        });
        cp5.addBang("allSelection").setPosition(280, 610).setSize(40, 40).setLabel("All (,)").addListener(new ControlListener() {
            @Override
            public void controlEvent(ControlEvent controlEvent) {
                matrix.all();
            }
        });

        cp5.addToggle("clearAfterColor").setPosition(230, 680).setSize(40, 40).setLabel("Clear after Color (+)");

        cp5.addBang("storeDefaultColor").setPosition(230, 680+70).setSize(40, 40).setLabel("store default Color");


        cp5.addLabel("recent colors:", 470, 610);
        for (int i = 0; i < MAX_COLOR_HISTORY; i++) {
            lastColors.add(cp5.addBang("colorX" + i).setPosition(470 + i * 43, 620).setSize(40, 40).setLabelVisible(false));
        }

        cp5.addLabel("base colors (F1 - F12):", 470, 680);
        for (int i = 0; i < MAX_COLOR_HISTORY; i++) {
            lastColors.add(cp5.addBang("colorX_p" + i).setPosition(470 + i * 43, 690).setSize(40, 40).setLabelVisible(false)
                    .setColorForeground(presetColors[i]).setColorActive(presetColors[i]));
        }

        cp5.addBang("randomColor").setPosition(470, 740).setSize(40, 40).setLabel("Random (*)");

        cp5.addSlider("fadetime").setPosition(470, 10).setSize(200, 40).setRange(0, 255).setValue(0);
        MyColorWheel wheel = new MyColorWheel(cp5, cp5.getDefaultTab(), "wheelcolor", 470, 70, 520, 520).setRGB(color(128, 0, 255)).setLabelVisible(false);
        wheel.addCallback(new CallbackListener() {
            @Override
            public void controlEvent(CallbackEvent theEvent) {
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
                        for(Cube cube: matrix.getCubeList()) {
                            if(cube.isSelected()) {
                                writeCube(cube);
                                cube.setLastUpdateMillis(millis());
                            }
                        }

                    }

                    if (clearAfterColor) {
                        matrix.clear();
                    }
                }
            }
        });


        PFont font = createFont("Verdana", 14);
        presetNameField = cp5.addTextfield("presetName").setPosition(10, 790).setSize(200, 30);

        cp5.addBang("presetupdate").setPosition(10, 840).setSize(40, 30).setLabel("Pre.Updt");
        cp5.addBang("presetnew").setPosition(60, 840).setSize(40, 30).setLabel("Pre.New").setColorForeground(color(0, 120, 0));
        cp5.addBang("presetdel").setPosition(110, 840).setSize(40, 30).setLabel("Pre.Del").setColorForeground(color(120, 0, 0));

        cp5.addBang("presetprev").setPosition(220, 840).setSize(40, 30).setLabel("Pre.Prev");
        cp5.addBang("presetnext").setPosition(270, 840).setSize(40, 30).setLabel("Pre.Next");


        surface.setTitle("Bjarneswuerfel");
        textSize(11);

        loadPresets();
    }

    public void storeDefaultColor() {
        if (sendEnable) {
            for(Cube cube: matrix.getCubeList()) {
                if(cube.isSelected()) {
                    gateway.write("C," + cube.getUid() + '\n');
                }
            }
        }
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

    public void randomColor() {
        Random random = new Random(millis());
        applyColorAndSend(color(random.nextInt(255),random.nextInt(255),random.nextInt(255)));
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

        // periodically refresh the cubes to make sure all is set as we wanted to
        if (!blackout && doRefreshing) {
            for(Cube cube: matrix.getCubeList()) {
                if (millis() - (cube.getLastUpdateMillis() + cube.getFadetime()) > REFRESH_TIMEOUT) {
                    if (sendEnable) {
                        writeCube(cube);
                    }
                    cube.setLastUpdateMillis(millis());
                }
            }
        }
    }

    public void presetprev() {
        int presetInt = (int) presetList.getValue();
        if (presetInt > 0) {
            presetList.setValue(presetInt - 1);
            presetLoad(presetInt - 1);
        }
    }

    public void presetnext() {
        int presetInt = (int) presetList.getValue();
        if (presetInt < presetList.getItems().size()) {
            presetList.setValue(presetInt + 1);
            presetLoad(presetInt + 1);
        }
    }

    public void keyPressed() {

        if(presetNameField.isFocus()) {
            return;
        }

        if (keyCode == RIGHT) {
            presetnext();
        } else if (keyCode == LEFT) {
            presetprev();
        } else if (keyCode == VK_NUMPAD0) {
            matrix.clear();
        } else if (keyCode == VK_SEPARATOR) {
            matrix.all();
        } else if (keyCode == VK_NUMPAD1) {
            matrix.odd();
        } else if (keyCode == VK_NUMPAD2) {
            matrix.even();
        } else if (keyCode == VK_NUMPAD3) {
            matrix.random();
        } else if (keyCode == VK_NUMPAD4) {
            matrix.selModulo(6);
        } else if (keyCode == VK_NUMPAD5) {
            matrix.selModulo(7);
        } else if (keyCode == VK_NUMPAD6) {
            // frei
        } else if (keyCode == VK_NUMPAD7) {
            matrix.selModulo(3);
        } else if (keyCode == VK_NUMPAD8) {
            matrix.selModulo(4);
        } else if (keyCode == VK_NUMPAD9) {
            matrix.selModulo(5);
        } else if(keyCode == VK_ADD) {
            Toggle tog = (Toggle) cp5.get("clearAfterColor");
            tog.setValue(!tog.getBooleanValue());
        } else if(keyCode == VK_MULTIPLY) {
            randomColor();
        }

        if(keyCode >= VK_F1 && keyCode <= VK_F12) {
            applyColorAndSend(presetColors[keyCode-VK_F1]);
        }

    }

    private void applyColorAndSend(int color) {
        matrix.setColorToSelection(color);

        if (sendEnable) {
            for(Cube cube: matrix.getCubeList()) {
                if(cube.isSelected()) {
                    writeCube(cube);
                    cube.setLastUpdateMillis(millis());
                }
            }
        }

        if (clearAfterColor) {
            matrix.clear();
        }
    }

    public void controlEvent(ControlEvent e) {

        if (e.isController()) {

            if (e.getController().getName().contentEquals("wheelcolor")) {
                // apply color to cubes while scrolling, data will be send to cubes by the refresh routine
                matrix.setColorToSelection((int) e.getValue());
            } else if (e.getController().getName().startsWith("colorX")) {

                applyColorAndSend((e.getController().getColor().getForeground()));

            } else if (e.getController().getName().contentEquals("fadetime")) {
                if (matrix != null) {
                    matrix.setFadetimeToSelection((int) e.getController().getValue());
                }
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

            for (int i = 0; i < matrix.getCubeList().size(); i++) {
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

                            if (sendEnable && currentColor != presetCube.getColor()) {
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
