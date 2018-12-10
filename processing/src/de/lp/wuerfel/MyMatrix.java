package de.lp.wuerfel;

import controlP5.ControlP5;
import controlP5.Controller;
import controlP5.ControllerView;
import de.lp.wuerfel.model.Cube;
import processing.core.PApplet;
import processing.core.PGraphics;

import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

import static processing.core.PApplet.nf;

/**
 * A matrix is a 2d array with a pointer that traverses through the matrix in a timed interval. if
 * an item of a matrix-column is active, the x and y position of the corresponding cell will trigger
 * an event and notify the program. see the ControlP5matrix example for more information.
 *
 * @example controllers/ControlP5matrix
 */
public class MyMatrix extends Controller<MyMatrix> {

    private List<Cube> _myCells;
    private int stepX;
    private int stepY;
    private boolean isPressed;
    private int _myCellX;
    private int _myCellY;
    private int sum;
    private int currentX = -1;
    private int currentY = -1;
    private int gapX = 1;
    private int gapY = 1;
    private Object _myPlug;
    private String _myPlugName;
    private int bg = 0x00000000;

    public MyMatrix(ControlP5 theControlP5, String theName, int theCellX, int theCellY, int theX, int theY, int theWidth, int theHeight) {
        super(theControlP5, theControlP5.getDefaultTab(), theName, theX, theY, theWidth, theHeight);
        theControlP5.register(theControlP5.papplet, theName, this);
        setGrid(theCellX, theCellY);

        _myPlug = cp5.papplet;
        _myPlugName = getName();
    }

    private MyMatrix setGrid(int theCellX, int theCellY) {
        _myCellX = theCellX;
        _myCellY = theCellY;
        sum = _myCellX * _myCellY;
        stepX = getWidth() / _myCellX;
        stepY = getHeight() / _myCellY;
        _myCells = new ArrayList<>();
        int cubeid = 1;
        for (int x = 0; x < _myCellX; x++) {
            for (int y = 0; y < _myCellY; y++) {
                _myCells.add(new Cube(cubeid, "Wuerfel " + cubeid));
                cubeid++;
            }
        }
        return this;
    }

    public void setColorToSelection(int newColor) {
        _myCells.stream().filter(Cube::isSelected).forEach(cube -> cube.setColor(newColor));
    }

    public void setFadetimeToSelection(int time) {
        _myCells.stream().filter(Cube::isSelected).forEach(cube -> cube.setFadetime(time));
    }

    public MyMatrix updateInternalEvents(PApplet theApplet) {
        setIsInside(inside());

        if (getIsInside()) {
            if (isPressed) {
                int tX = (int) ((theApplet.mouseX - x(position)) / stepX);
                int tY = (int) ((theApplet.mouseY - y(position)) / stepY);

                if (tX != currentX || tY != currentY) {
                    tX = PApplet.min(PApplet.max(0, tX), _myCellX);
                    tY = PApplet.min(PApplet.max(0, tY), _myCellY);

                    Cube cube = _myCells.get(tY * _myCellX + tX);

                    cube.setSelected(!cube.isSelected());

                    currentX = tX;
                    currentY = tY;
                }
            }
        }
        return this;
    }

    protected void onEnter() {
        isActive = true;
    }

    protected void onLeave() {
        isActive = false;
    }

    public void mousePressed() {
        isActive = getIsInside();
        if (getIsInside()) {
            isPressed = true;
        }
    }

    protected void mouseReleasedOutside() {
        mouseReleased();
    }

    public void mouseReleased() {
        if (isActive) {
            isActive = false;
        }
        isPressed = false;
        currentX = -1;
        currentY = -1;
    }

    @Override
    public MyMatrix setValue(float theValue) {
        _myValue = theValue;
        broadcast(FLOAT);
        return this;
    }


    @Override
    public MyMatrix update() {
        return setValue(_myValue);
    }


    public MyMatrix plugTo(Object theObject) {
        _myPlug = theObject;
        return this;
    }

    public MyMatrix plugTo(Object theObject, String thePlugName) {
        _myPlug = theObject;
        _myPlugName = thePlugName;
        return this;
    }


    public void clear() {
        _myCells.forEach(cube -> cube.setSelected(false));
    }

    public void odd() {
        boolean odd = true;
        for (Cube cube : _myCells) {
            cube.setSelected(odd);
            odd = !odd;
        }
    }

    public void even() {
        boolean odd = false;
        for (Cube cube : _myCells) {
            cube.setSelected(odd);
            odd = !odd;
        }
    }

    public void all() {
        _myCells.forEach(cube -> cube.setSelected(true));
    }


    @Override
    public MyMatrix updateDisplayMode(int theMode) {
        _myDisplayMode = theMode;
        switch (theMode) {
            case (DEFAULT):
                _myControllerView = new MatrixView();
                break;
            case (IMAGE):
            case (SPRITE):
            case (CUSTOM):
            default:
                break;
        }
        return this;
    }

    public List<Cube> getCubeList() {
        return _myCells;
    }

    public List<Cube> getCubeListSelected() {
        return _myCells.stream().filter(Cube::isSelected).collect(Collectors.toList());
    }


    class MatrixView implements ControllerView<MyMatrix> {

        public void display(PGraphics theGraphics, MyMatrix theController) {
            theGraphics.noStroke();
            theGraphics.fill(bg);
            theGraphics.rect(0, 0, getWidth(), getHeight());

            float gx = gapX / 2;
            float gy = gapY / 2;
            for (int x = 0; x < _myCellX; x++) {
                for (int y = 0; y < _myCellY; y++) {

                    Cube cube = _myCells.get(y * _myCellX + x);

                    if (cube.isSelected()) {
                        theGraphics.fill(color.getActive());
                    } else {
                        theGraphics.fill(color.getBackground());
                    }

                    theGraphics.rect(x * stepX + gx, y * stepY + gy, stepX - gapX, stepY - gapY);

                    theGraphics.fill(0);
                    theGraphics.rect((x * stepX + gx) + 1, (y * stepY + gy) + 1, stepX - gapX - 2, stepY - gapY - 2);

                    theGraphics.fill(cube.getColor());
                    theGraphics.rect((x * stepX + gx) + 3, (y * stepY + gy) + 3, stepX - gapX - 6, stepY - gapY - 6);


                    float sizeOld = theGraphics.textSize;


                    theGraphics.textFont(cp5.getFont().getFont());
                    theGraphics.textSize(12);
                    theGraphics.fill(255);
                    theGraphics.text(cube.getUid() + "", (x * stepX + gx) + 3, (y * stepY + gy) + 13);
                    theGraphics.textSize(10);
                    theGraphics.text(nf(cube.getFadetime() / 100f, 1,1) + "s", (x * stepX + gx) + 3, (y * stepY + gy) + 25);
                    theGraphics.textSize(sizeOld);
                }
            }


        }
    }
}
