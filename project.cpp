#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

Mat apply_op(const Mat& img, const string& op, int beta, int thresh_val) {
    Mat out;
    if (op == "Grayscale") {
        out = Mat(img.rows, img.cols, CV_8UC1);
        for (int y = 0; y < img.rows; y++) {
            for (int x = 0; x < img.cols; x++) {
                Vec3b p = img.at<Vec3b>(y, x);
                out.at<uchar>(y, x) = static_cast<uchar>(0.299 * p[2] + 0.587 * p[1] + 0.114 * p[0]);
            }
        }
    } else if (op == "Brightness") {
        out = Mat(img.rows, img.cols, img.type());
        for (int y = 0; y < img.rows; y++) {
            for (int x = 0; x < img.cols; x++) {
                Vec3b p = img.at<Vec3b>(y, x);
                for (int c = 0; c < 3; c++) {
                    int val = static_cast<int>(p[c]) + beta;
                    out.at<Vec3b>(y, x)[c] = static_cast<uchar>(min(255, max(0, val)));
                }
            }
        }
    } else if (op == "Flip") {
        out = Mat(img.rows, img.cols, img.type());
        for (int y = 0; y < img.rows; y++) {
            for (int x = 0; x < img.cols; x++) {
                out.at<Vec3b>(y, img.cols - 1 - x) = img.at<Vec3b>(y, x);
            }
        }
    } else if (op == "BW") {
        Mat gray = apply_op(img, "Grayscale", beta, thresh_val);
        out = Mat(img.rows, img.cols, CV_8UC1);
        for (int y = 0; y < gray.rows; y++) {
            for (int x = 0; x < gray.cols; x++) {
                uchar g = gray.at<uchar>(y, x);
                out.at<uchar>(y, x) = (g >= thresh_val) ? 255 : 0;
            }
        }
    } else if (op == "Rotate") {
        out = Mat(img.cols, img.rows, img.type());
        for (int y = 0; y < img.rows; y++) {
            for (int x = 0; x < img.cols; x++) {
                out.at<Vec3b>(x, img.rows - 1 - y) = img.at<Vec3b>(y, x);
            }
        }
    }
    return out;
}

void save_matrix_report(const Mat& orig, const Mat& proc, const string& op, double ms, const string& path) {
    ofstream f(path);
    f << "ORIG_SIZE=" << orig.rows << "x" << orig.cols << "\n";
    f << "PROC_SIZE=" << proc.rows << "x" << proc.cols << "\n";
    f << "PIXELS=" << (orig.rows * orig.cols) << "\n";
    f << "TIME_MS=" << fixed << setprecision(1) << ms << "\n";
    f << "MATRIX_START\n";
    f << "=== 2D PIXEL MATRIX SAMPLE (4x4 Center) ===\n";
    f << "[BEFORE - RGB]:\n";
    int cy = orig.rows / 2, cx = orig.cols / 2;
    for (int y = max(0, cy - 2); y < min(orig.rows, cy + 2); y++) {
        f << "[ ";
        for (int x = max(0, cx - 2); x < min(orig.cols, cx + 2); x++) {
            Vec3b p = orig.at<Vec3b>(y, x);
            f << "(" << (int)p[2] << "," << (int)p[1] << "," << (int)p[0] << ") ";
        }
        f << "]\n";
    }
    f << "\n[AFTER]:\n";
    int acy = proc.rows / 2, acx = proc.cols / 2;
    for (int y = max(0, acy - 2); y < min(proc.rows, acy + 2); y++) {
        f << "[ ";
        for (int x = max(0, acx - 2); x < min(proc.cols, acx + 2); x++) {
            if (proc.channels() == 1) {
                f << setw(3) << (int)proc.at<uchar>(y, x) << " ";
            } else {
                Vec3b p = proc.at<Vec3b>(y, x);
                f << "(" << (int)p[2] << "," << (int)p[1] << "," << (int)p[0] << ") ";
            }
        }
        f << "]\n";
    }
    f.close();
}

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    string mode = argv[1];
    string in_path = argv[2];
    Mat img = imread(in_path);
    if (img.empty()) return 1;

    if (mode == "single") {
        string out_path = argv[3];
        string rep_path = argv[4];
        string op = argv[5];
        int beta = stoi(argv[6]);
        int thresh_val = stoi(argv[7]);
        auto t1 = chrono::high_resolution_clock::now();
        Mat proc = apply_op(img, op, beta, thresh_val);
        auto t2 = chrono::high_resolution_clock::now();
        double ms = chrono::duration<double, milli>(t2 - t1).count();
        imwrite(out_path, proc);
        save_matrix_report(img, proc, op, ms, rep_path);
    } else if (mode == "all4") {
        string dir = argv[3];
        int beta = stoi(argv[4]);
        int thresh_val = stoi(argv[5]);
        imwrite(dir + "/out_gray.png", apply_op(img, "Grayscale", beta, thresh_val));
        imwrite(dir + "/out_bright.png", apply_op(img, "Brightness", beta, thresh_val));
        imwrite(dir + "/out_flip.png", apply_op(img, "Flip", beta, thresh_val));
        imwrite(dir + "/out_bw.png", apply_op(img, "BW", beta, thresh_val));
    }
    return 0;
}
