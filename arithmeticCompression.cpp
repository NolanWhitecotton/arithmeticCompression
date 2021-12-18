#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>

using namespace std;
//https://go-compression.github.io/algorithms/arithmetic/

class tableEntry {
public:
    uint32_t size; // the proportion of the total table which it takes up
    uint8_t data; // the data that this adds as the table is traversed
    tableEntry(uint32_t size, uint8_t data) : size(size), data(data) {};
    void dump() {
        cout << (uint16_t)data << ", occurs " << size << endl;
    }
};

//returns -1 if fraction 1 is less, 0 if they're equal, and 1 if fraction 1 is greater
int compareFractions(uint32_t n1, uint32_t d1, uint32_t n2, uint32_t d2){
    //https://janmr.com/blog/2014/05/comparing-rational-numbers-without-overflow/
    
    //integer division comparison
    uint32_t q1 = n1 / d1;
    uint32_t q2 = n2 / d2;
    
    uint32_t r1 = n1 - q1 * d1;
    uint32_t r2 = n2 - q2 * d2;

    if (q1 > q2) return -1; 
    if (q1 < q2) return 1;

    if (r1 == 0 && r2 == 0) return 0;
    if (r1 == 0) return 1;
    if (r2 == 0) return -1;

    //call recursively, flipping the fraction and using the remainder
    return -1*compareFractions(d1, r1, d2, r2);
}

//TODO, make reads and writes work independantly of endianness
void writeMagicNumber(ofstream &outfile) {
    const uint32_t magicNumber = 0x082c8bf1;
    outfile.write(reinterpret_cast<const char*>(&magicNumber), sizeof(magicNumber));
}

class table {
public:
    uint32_t maxTableSize = 0;
    vector<tableEntry> entries;

    //returns an allocated cstring that is of size length
    uint8_t* getCharsAt(uint32_t dividend, uint32_t divisor, int length) {
        uint8_t* output = new uint8_t[length];

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
                    int entryStart = sum - entries[i].size;

                    double pos2 = (pos - entryStart) / ((double)sum - entryStart) * maxTableSize;
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


    //this is purely used for debugging, do not use
    //converts a string to an allocated uint8_t array
    uint8_t* convertStringToUint8Arr(string s) {
        uint8_t* output = new uint8_t[s.length()];

        for (int i = 0; i < s.length(); i++) {
            output[i] = s[i];
        }
        return output;
    }

    //returns the numerator for a valid encoding of s in the table (the denom is always UINT8_MAX)
    //only does a single encoding, returns 0 if encoding fails, or if 0 is the found encoding
    uint32_t encode(uint8_t* buffer, int length, uint32_t start=0, uint32_t end=UINT32_MAX) {
        //https://www.geeksforgeeks.org/binary-search/
        uint32_t mid = start + (end - start) / 2;

        if (end >= start) {
            uint8_t* curData = getCharsAt(mid, UINT32_MAX, length);
            int compData = compareBinaryData(curData, buffer, length);
            delete[] curData;
            if (compData == 0) { //if mid == the target data
                return mid;
            }

            if (compData > 0) { //if mid > the target data
                return encode(buffer, length, start, mid-1);
            }

            return encode(buffer, length, mid+1, end);
        }
        
        return 0;//fail to encode
    }
    
    //read through the entire stream and build the table entries
    void buildTableFromDataStream(istream& s, ostream &output) {
        vector<int> byteCounter(0);
        byteCounter.resize(256);
        int totalBytes = 0;

        uint8_t byte;
        
        while (s.read((char*)&byte, sizeof(char))) {
            if (s.gcount() != sizeof(char))
                break;
            byteCounter.at((uint16_t)byte)++;
            totalBytes++;
        }

        //create table
        for (int i = 0; i < byteCounter.size(); i++) {
            if (byteCounter[i] != 0) {
                tableEntry ent(byteCounter[i], i);
                entries.push_back(ent);
            }
        }
        maxTableSize = totalBytes;

        s.clear(); //reset the EOF state
        s.seekg(0); //reset the file pointer, so that the file can be read again

        cout << "Number of unique bytes: " << entries.size() << endl;
        
        //write table to file
        uint32_t entryCount = entries.size();
        output.write(reinterpret_cast<const char*>(&entryCount), sizeof(uint32_t));
        for (int i = 0; i < entryCount; i++) {
            output.write(reinterpret_cast<const char*>(&entries[i].size), sizeof(uint32_t));
            output.write(reinterpret_cast<const char*>(&entries[i].data), sizeof(uint8_t));
        }
    }

    void dumpTable() {
        for (int i = 0; i < entries.size(); i++) {
            entries[i].dump();
        }
    }

    void buildTableFromArchiveStream(ifstream &s) {
        //validate magicNumber
        uint32_t fileMagicNumber = 0;
        uint32_t magicNumber = 0x082C8BF1;
        s.read((char*)&magicNumber, 4);
        if (fileMagicNumber == magicNumber) {
            cout << "Invalid file type, magic number is incorrect." << endl;
            exit(1);
        }

        //get the number of table entries in the archive
        uint32_t entryCount = 0;
        s.read((char*)&entryCount, sizeof(uint32_t));

        //read the entries
        int totalBytes = 0;
        for (int i = 0; i < entryCount; i++) {
            //read entry
            uint32_t size = 0;
            uint8_t data = 0;
            s.read((char*)&size, sizeof(uint32_t));
            s.read((char*)&data, sizeof(uint8_t));

            
            //add entry
            tableEntry ent(size, data);
            entries.push_back(ent);

            totalBytes += size;
        }

        maxTableSize = totalBytes;

        cout << "Number of unique bytes: " << entries.size() << endl;
    }

    //reads input until end of file and decodes the data and writes the decoded data to output
    void decodeFromDataStream(istream& input, ostream& output) {
        //until eof
        uint8_t length;
        uint32_t dividend;

        bool doneReading = false;
        while (!doneReading) {
            //read input
            input.read((char*)&length, sizeof(length));
            input.read((char*)&dividend, sizeof(dividend));

            //check for EOF, this should never happen on a valid archive
            if (input.eof()) {
                doneReading = true;
                break;
            }

            //get decoded value
            uint8_t* data = getCharsAt(dividend, UINT32_MAX, length);

            //write decoded value
            output.write((char*)data, length);

            delete[] data;
        }
    }

    //reads a stream and breaks it into buffers that will then get individually encoded
    void encodeFromStream(istream& input, ostream& output) {
        const int bufferSize = 500;
        uint8_t buffer[bufferSize];
        while (input.read((char*)buffer, bufferSize)) {
            encodeFromUintArr((uint8_t*)buffer, bufferSize, output); //encode a full read
        }
        if(input.gcount() > 0)
            encodeFromUintArr((uint8_t*)buffer, input.gcount(), output); //encode a partial read
    }

    //break input into the largest chunks that can be encoded and encode
    void encodeFromUintArr(uint8_t* input, int length, ostream &output) {
        vector<uint32_t> encodedData;
        vector<uint8_t> encodedLengths;

        int start = 0,
            end = 1;
        int lastEncodeLen=end;
        uint32_t lastSuccessfulEncoding = 0;

        bool EOB = false; //end of buffer
        while (!EOB) {
            while (true) {//can be encoded
                uint32_t encoded = encode(input+start, end);

                uint8_t* data = getCharsAt(encoded, UINT32_MAX, length);

                //TODO check the contional in this if statement, its a bit wacky
                if ((encoded == 0 || compareBinaryData(data+start, input+start, end-start)) || //if the data failed to encode
                        start + end - 1 == length) { //or if the encoding is at the end of the buffer

                    if (start + end - 1 == length) {//if at the end of the buffer
                        EOB = true;
                    } else {
                        //update start and end
                        start += end - 1;
                        end = 1;
                    }

                    break;
                }
                lastSuccessfulEncoding = encoded;
                lastEncodeLen = end;
                end++;
            }
            encodedData.push_back(lastSuccessfulEncoding);
            encodedLengths.push_back(lastEncodeLen);
        }

        //write the compressed buffer to output in binary form
        for (int i = 0; i < encodedData.size(); i++) {
            output.write(reinterpret_cast<const char*>(&encodedLengths[i]), sizeof(encodedLengths[i]));
            output.write(reinterpret_cast<const char*>(&encodedData[i]), sizeof(encodedData[i]));
        }
    }
};

int main() {

    ifstream input("test.txt", ios::binary | ios::in);
    ofstream outfile("output.txt", ios::binary | ios::out);

    //write file header
    writeMagicNumber(outfile);

    //create, build, and write the table
    table t;
    t.buildTableFromDataStream(input, outfile);

    //print summary of the table
    for (int one = 0; one < 100; one++) {
        uint8_t* data = t.getCharsAt(one, 100, 1);
        cout.write((char*)data,1);
        delete[] data;
    }
    cout << endl;

    t.encodeFromStream(input, outfile);
    t.dumpTable();

    input.close();
    outfile.close();

    //open and read the file
    table t2;

    ifstream infile("output.txt", ios::binary | ios::in);
    ofstream decodedOutFile("decodedInput.txt", ios::binary | ios::out);

    t2.buildTableFromArchiveStream(infile);
    t2.dumpTable();
    t2.decodeFromDataStream(infile, decodedOutFile);

    infile.close();
    decodedOutFile.close();

    cout << "done." << endl;
}
