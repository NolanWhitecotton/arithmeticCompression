#include <iostream>
#include <vector>
#include <sstream>

using namespace std;
//https://go-compression.github.io/algorithms/arithmetic/

class tableEntry {
public:
    uint32_t size; // the proportion of the total table which it takes up
    uint8_t data; // the data that this adds as the table is traversed
    tableEntry(uint32_t size, uint8_t data) : size(size), data(data) {};
};

//returns -1 if fraction 1 is less, 0 if they're equal, and 1 if fraction 1 is greater
int compareFractions(uint32_t dividend1, uint32_t divisor1, uint32_t dividend2, uint32_t divisor2){
    //TODO implement compareFractions
    return 0;
}

class table {
public:
    uint32_t maxTableSize;
    vector<tableEntry> entries;

    //returns an allocated cstring that is of size length
    uint8_t* getCharsAt(uint32_t dividend, uint32_t divisor, int length) {
        uint8_t* output = new uint8_t[length+1];
        output[length] = '\0';

        double d = (double)dividend / divisor;//TODO probably shouldnt convert this to a double at any point for rounding errors
        double pos = maxTableSize * d;//the position in the table that is being looked for

        for (int o = 0; o < length; o++) {//for every char to be found
            //try to find the char that is represented by pos
            int sum = 0;
            for (int i = 0; i < entries.size(); i++) {//for every table entry
                sum += entries[i].size;
                if (sum >= pos) {
                    //print found byte
                    output[o] = entries[i].data;

                    //prep for next byte
                    //determine how far pos overshoots the found character
                    int entryStart = sum - entries[i].size;//TODO store previous int or smth so this doesn't need to be calcualted every time

                    double pos2 = (pos - entryStart) / (sum - entryStart) * maxTableSize;
                    pos = pos2;
                    break;
                }
            }
        }
        return output;
    }

    //returns the relation between two strings (-1 for first one is smaller, 0 equal, 1 for first is larger)
    int compareBinaryData(uint8_t* str1, uint8_t* str2, int length) {
        for (int i = 0; i < length; i++) {
            int diff = (int)str1[i] - str2[i];

            if (diff != 0) {
                return diff;
            }
        }
        return 0;
    }

    uint8_t* convertStringToUint8Arr(string s) {
        uint8_t* output = new uint8_t[s.length()+1];
        output[s.length()] = '\0';

        for (int i = 0; i < s.length(); i++) {
            output[i] = s[i];
        }
        return output;
    }

    //returns the numerator for a valid encoding of s in the table (the denom is always UINT8_MAX)
    uint32_t encode(string s, uint32_t start=0, uint32_t end=UINT32_MAX) {
        //https://www.geeksforgeeks.org/binary-search/
//        cout << "encode('" << s << "', " << start << ", " << end << ");" << endl;
        uint32_t mid = start + (end - start) / 2;
//        cout << "  mid: "<< mid << endl;
        if (end >= start) {
            uint8_t* curData = getCharsAt(mid, UINT32_MAX, s.length());
            cout << "  curData: " << curData << endl;
            uint8_t* converted = convertStringToUint8Arr(s);
            int compData = compareBinaryData(curData, converted, s.length());
            delete[] converted;
//            cout << "  comparison: " << compData << endl;
            if (compData == 0) { //if mid == the target data
                delete[] curData;
                return mid;
            }

            if (compData > 0) { //if mid > the target data
                delete[] curData;
                return encode(s, start, mid-1);
            }

            delete[] curData;
            return encode(s, mid+1, end);
        }
        
        return 0;//fail to encode

    }
};

int main() {
    //read file
    uint8_t byte;
    stringstream input("testdata");

    vector<int> byteCounter(0);
    byteCounter.resize(255);
    int totalBytes = 0;

    while (input >> byte) {
        byteCounter[byte]++;
        totalBytes++;
    }

    //create table
    table t;
    for (int i = 0; i < byteCounter.size(); i++) {
        if (byteCounter[i] != 0) {
            tableEntry ent(byteCounter[i], i);
            t.entries.push_back(ent);
        }
    }
    t.maxTableSize = totalBytes;


    cout << "Number of unique bytes: " << t.entries.size() << endl;

    for (int one = 0; one < 100; one++) {
        uint8_t* data = t.getCharsAt(one, 100, 1);
        cout << data;
        delete[] data;
    }
    cout << endl;

    //zoom in on the 1s (should be idential to the one above)
    //TODO this doesn't take into account the size of the element that we are zooming in on 
    for (int one = 0; one < 100; one++) {
        uint8_t* data = t.getCharsAt(one, 1000, 2);
        cout << data[1];
        delete[] data;
    }
    cout << endl;

    
    //encode a value then decode it again
    string strToEncode = "test";
    uint32_t encoding = t.encode(strToEncode);
    cout << "encoding for '" << strToEncode << "': " << encoding  << "/" << UINT32_MAX << endl;

    uint8_t* data = t.getCharsAt(encoding, UINT32_MAX, strToEncode.length());
    cout << "decoded value: " << data << endl;
    delete[] data;

    //cout << t.getCharsAt(1, 5, 20) << endl;

    cout << "done." << endl;
}
