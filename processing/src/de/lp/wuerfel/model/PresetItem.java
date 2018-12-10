package de.lp.wuerfel.model;

import java.util.ArrayList;
import java.util.List;

public class PresetItem {

    private String name = null;
    private List<BaseCube> presetCubes = new ArrayList<>();

    public PresetItem() {
    }

    public PresetItem(String name) {
        this.name = name;
    }

    public Boolean load(String lineFromFile) {
        int atPos = lineFromFile.indexOf("@");
        if(atPos > 0) {
            this.name = lineFromFile.substring(0, atPos);
            String rest = lineFromFile.substring(atPos+1);
            String cubes[] = rest.split(";");

            for(int i = 0; i < cubes.length; i++) {
                String parts[] = cubes[i].split(":");
                if(parts.length == 2) {
                    Cube leCube = new Cube();
                    leCube.setColor(Integer.parseInt(parts[0]));
                    leCube.setFadetime(Integer.parseInt(parts[1]));
                    presetCubes.add(leCube);
//                } else {
//                    System.out.println("Format Error, cannot load cube");
//                    return false;
                }
            }
            return true;
        }
        System.out.println("Format Error, cannot load cubes");
        return false;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public List<BaseCube> getPresetCubes() {
        return presetCubes;
    }

    public String toString() {
        StringBuilder listString = new StringBuilder();

        listString.append(name);
        listString.append("@");

        for(BaseCube cube: presetCubes) {
            listString.append(cube.getColor());
            listString.append(":");
            listString.append(cube.getFadetime());
            listString.append(";");
        }

        return listString.toString();
    }

}
