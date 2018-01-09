#ifndef MAIN_H
#define MAIN_H
#include "ui_mainwindow.h"
#include "canvas.h"
#include <vector>
#include <queue>
using namespace std;

QT_BEGIN_NAMESPACE

#define INITIAL 0
#define ACTIVE 1
#define EXPANDED 2
#define INF 1000000
#define CLOSETHRES 6
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

struct Node {
    double linkCost[9];
    int state;
    double totalCost;
    Node *prevNode;
    int prevNodeNum;
    int col,row;
    int num;
    bool operator < (const Node &a) const {
        return totalCost>a.totalCost;
    }
};

class Window : public QMainWindow, Ui_MainWindow
{
    Q_OBJECT
public:
    Canvas *canvas;
    Window();
    void openFile(QString filename);
private slots:
    void openFile();
    void saveFile();
    void getGradientMap();
    void startScissor();
    void onMousePress(int x, int y);
    void onMouseMove(int x, int y);
private:
    bool init_seed;     // If first seed placed
    bool is_start;
    bool is_closed;     // If boundary is closed
    QImage image;
    QImage grad_image;
    int seed_num;
    int seed_x;
    int seed_y;

    vector<struct Node> node_vector;
    vector<QPoint> seed_vector;         // contains seed points (where mouse is clicked)
    vector<QPoint> shortPath_vector;    // path from target point to seed point
    vector<vector<QPoint>> fullPath_vector; // vector of shortPath_vectors

    void connectSignals();
    void initVector(int x1, int y1, int x2, int y2);
    void closeDetect(int x, int y);
    void getMask(int x, int y);
};

void floodfill(QImage &img, int x, int y);

QT_END_NAMESPACE
#endif // MAIN_H