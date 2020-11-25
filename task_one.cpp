#include <stdlib.h>
#include <iostream>
#include <functional>
#include <fstream>
#include <string>

using namespace std;

void forRange(int from, int to, function <void(int)> callback) {
    for (int x = from; x < to; x++) {
        callback(x);
    }
}

int num_length(int num) {
    int length = 0;

    while (num) {
      length++;
      num /= 10;
    }

    return length;
}

enum FileMode {
    read,
    write
};

class File {
    string _path;
    fstream file;

    void check_file_good() {
        if (file.good()) {
            return;
        }

        throw "Something went wrong with file located at " + _path;
    }

public:
    File(string path, string file_name, FileMode mode) {
        if (path.back() == '/') {
            _path = path + file_name;
        }
        else {
            _path = path + '/' + file_name;
        }

        if (mode == read) {
            file.open(_path);
        }
        else if (mode == write) {
            file.open(_path, fstream::out);
        }
        
        check_file_good();
    }

    File(string path) {
        _path = path;

        file.open(path);
        check_file_good();
    }

    ~File() {
        file.close();
    }

    void write_line(string line) {
        check_file_good();

        file << line << '\n';
    }

    void write_line(int number) {
        check_file_good();

        file << number << '\n';
    }

    void read_line(function <void(int, int)> on_value) {
        string line;
        string collected;
        int read_so_far = 0;

        getline(file, line);
 
        for (auto i = line.begin(); i != line.end(); i++) {
            if (*i == ' ') {
                on_value(stoi(collected), read_so_far);

                collected = "";
                read_so_far++;
            }
            else {
                collected += *i;
            }
        }

        if (!collected.empty()) {
            on_value(stoi(collected), read_so_far);
        }

    }

    void read_lines(function <void(int, int)> on_value) {
        int read_so_far = 0;

        while (file.good()) {
            read_line([&read_so_far, on_value](int value, int index) -> void {
                on_value(value, read_so_far);
                read_so_far++;
            });
        }
    }

};

class Matrix {
    int** _matrix;
    int _y;
    int _x;

    int** _create_matrix(int x, int y) {
        int** matrix = new int* [y];
        _y = y;
        _x = x;

        for (int i = 0; i < y; i++) {
            matrix[i] = new int[x];
        }

        return matrix;
    }

    Matrix* _duplicate() {
        return new Matrix(_x, _y);
    }

    void _throw_if_dimensions_different(int x, int y, string msg) const {
        if (cols() == x && rows() == y) {
            return;
        }
        throw msg;
    }

    void _throw_if_dimensions_different(Matrix matrixB, string msg) const {
        if (matrixB.cols() == cols() && matrixB.rows() == rows()) {
            return;
        }
        throw msg;
    }

public:
    Matrix(int x) {
        _y = x;
        _x = x;
 
        _matrix = _create_matrix(_x, _x);
        map([](int x, int y) -> int { return 0; });
    }

    Matrix(int x, int y) {
        _y = y;
        _x = x;

        _matrix = _create_matrix(_x, _y);
        map([](int x, int y) -> int { return 0; });
    }

    Matrix(string path, string file_name) {
        File file(path, file_name, read);
 
        file.read_line([this](int value, int index) -> void {
            if (index == 0) { _x = value; }
            else { _y = value; }
         });

        _matrix = _create_matrix(_x, _y);
 
        file.read_lines([this](int value, int index) -> void {
           set(index % _x, (index) / _x, value);
        });
    }

    Matrix(const Matrix& matrix) {
       _matrix = _create_matrix(matrix.cols(), matrix.rows());

       map([this, &matrix](int x, int y) -> int {
           return matrix.get(x, y);
       });
    }

    ~Matrix() {
        forRange(0, _y, [this](int y) -> void {
            delete[] _matrix[y];
        });
        delete[] _matrix;
    }

    void print() const {
        forRange(0, _y, [this](int y) -> void {
            cout << "[";

            forRange(0, _x, [this, y](int x) -> void {
                int current_number = get(x, y);

                if (x) {
                    forRange(0, 2, [](int i) -> void { cout << " "; });
                }

                cout << current_number;

                if (x == cols() - 1) {
                    cout << "]\n";
                }
            });
        });
    }

    void set(int x, int y, int value) {
        _matrix[y][x] = value;
    }

    int get(int x, int y) const {
        return _matrix[y][x];
    }

    int cols() const {
        return _x;
    }

    int rows() const {
        return _y;
    }

    void store(string path, string file_name) {
        File file(path, file_name, write);

        file.write_line(to_string(_x) + " " + to_string(_y));
      
        forEach([this, &file](int x, int y) -> void {
            file.write_line(get(x, y));
        });
    }

    void forEach(function <void(int, int)> callback) {
        forRange(0, _y, [this, callback](int y) -> void {
            forRange(0, _x, [callback, y](int x) -> void {
                callback(x, y);
            });
        });
    }

    Matrix* map(function< int(int, int)> callback) {
        forEach([this, callback](int x, int y) -> void {
            set(x, y, callback(x, y));
        });

        return this;
    }

    Matrix* add(Matrix matrixB) {
        _throw_if_dimensions_different(matrixB, "Can't add matrices of different dimensions");
     
        return _duplicate()->map([this, &matrixB](int x, int y) -> int {
            return get(x, y) + matrixB.get(x, y);
        });
    }

    Matrix* subtract(Matrix matrixB) {
        _throw_if_dimensions_different(matrixB, "Can't subtract matrices of different dimensions");

        return _duplicate()->map([this, &matrixB](int x, int y) -> int {
            return get(x, y) - matrixB.get(x, y);
        });
    }

    Matrix* multiply(Matrix matrixB) {
        _throw_if_dimensions_different(matrixB.rows(), rows(), "Can't multiply matrices with incorrect dimensions");

        Matrix* product = new Matrix(matrixB.cols(), rows());

        return product->map([this, &matrixB](int x, int y) -> int {
            int sum = 0;

            forRange(0, cols(), [this, &sum, x, y, &matrixB](int i) -> void {
                sum += get(i, y) * matrixB.get(x, i);
            });
         
            return sum;
        });
    }
};


int main() {

    cout << "Creating matrices: \n";
    cout << "Matrix A: \n";
    Matrix matrixA(3, 5);

    matrixA.set(0, 0, 1);
    matrixA.set(1, 1, 2);
    matrixA.set(2, 2, 3);
    matrixA.set(1, 3, 4);
    matrixA.set(0, 4, 5);

    matrixA.print();

    cout << "\nMatrix B: \n";
    Matrix matrixB(7, 3);

    matrixB.set(0, 0, 1);
    matrixB.set(1, 1, 2);
    matrixB.set(2, 2, 3);
    matrixB.set(3, 1, 4);
    matrixB.set(4, 0, 5);
    matrixB.set(5, 1, 6);
    matrixB.set(6, 2, 7);

    matrixB.print();

    cout << "\nMatrix C: \n";
    Matrix matrixC(3, 5);

    matrixC.set(2, 0, 5);
    matrixC.set(1, 1, 4);
    matrixC.set(0, 2, 3);
    matrixC.set(1, 3, 2);
    matrixC.set(2, 4, 1);

    matrixC.print();

    cout << "\nAdding A to C\n";
    Matrix* sum = matrixA.add(matrixC);
    sum->print();
    delete sum;

    cout << "\nSubtracting C from A\n";
    Matrix* diff = matrixA.subtract(matrixC);
    diff->print();
    delete diff;

    cout << "\nMultiplying A by B\n";
    Matrix* product = matrixA.multiply(matrixB);
    product->print();
    delete product;

    cout << "\nSaving A to a file and reading it\n";
    matrixA.store(".", "matrix");
    Matrix matrixD(".", "matrix");
    matrixD.print();

    cout << "\nCopying via copy constructor\n";
    Matrix matrixE(matrixD);
    matrixE.print();

    return 0;
}
