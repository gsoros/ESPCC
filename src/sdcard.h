#ifndef __sdcard_h
#define __sdcard_h

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include "atoll_fs.h"

#define SCK 18
#define MISO 19
#define MOSI 23
#define CS 5

class SdCard : public Atoll::Fs {
   public:
    SPIClass spi = SPIClass(VSPI);

    SdCard() {}
    virtual ~SdCard() {}

    void setup() {
        spi.begin(SCK, MISO, MOSI, CS);
        // if (!SD.begin(CS, spi, 80000000)) {
        if (!SD.begin(CS, spi)) {
            log_i("Card Mount Failed");
            return;
        }

        uint8_t cardType = SD.cardType();
        if (cardType == CARD_NONE) {
            log_i("No SD card attached");
            return;
        }
        mounted = true;

        log_i("Card Type: ");
        if (cardType == CARD_MMC) {
            log_i("MMC");
        } else if (cardType == CARD_SD) {
            log_i("SDSC");
        } else if (cardType == CARD_SDHC) {
            log_i("SDHC");
        } else {
            log_i("Unknown");
        }

        log_i("Card Size: %lluMB", SD.cardSize() / (1024 * 1024));
        log_i("Total space: %lluMB", SD.totalBytes() / (1024 * 1024));
        log_i("Used space: %lluMB", SD.usedBytes() / (1024 * 1024));
    }

    void test() {
        listDir(SD, "/", 0);
        createDir(SD, "/testing");
        listDir(SD, "/", 0);
        removeDir(SD, "/testing");
        listDir(SD, "/", 2);
        writeFile(SD, "/testing.txt", "Hello ");
        appendFile(SD, "/testing.txt", "World!");
        readFile(SD, "/testing.txt");
        deleteFile(SD, "/testing2.txt");
        renameFile(SD, "/testing.txt", "/testing2.txt");
        readFile(SD, "/testing2.txt");
        testFileIO(SD, "/testing2.txt");
        deleteFile(SD, "/testing2.txt");
    }

    void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
        log_i("Listing directory: %s", dirname);

        File root = fs.open(dirname);
        if (!root) {
            log_i("Failed to open directory");
            return;
        }
        if (!root.isDirectory()) {
            log_i("Not a directory");
            return;
        }

        File file = root.openNextFile();
        while (file) {
            if (file.isDirectory()) {
                log_i("Dir: %s", file.name());
                if (levels) {
                    listDir(fs, file.name(), levels - 1);
                }
            } else {
                log_i("File: %s, size: %d", file.name(), file.size());
            }
            file = root.openNextFile();
        }
    }

    void createDir(fs::FS &fs, const char *path) {
        log_i("Creating Dir: %s", path);
        if (fs.mkdir(path)) {
            log_i("Dir created");
        } else {
            log_i("mkdir failed");
        }
    }

    void removeDir(fs::FS &fs, const char *path) {
        log_i("Removing Dir: %s", path);
        if (fs.rmdir(path)) {
            log_i("Dir removed");
        } else {
            log_i("rmdir failed");
        }
    }

    void readFile(fs::FS &fs, const char *path) {
        log_i("Reading file: %s", path);

        File file = fs.open(path);
        if (!file) {
            log_i("Failed to open file for reading");
            return;
        }

        log_i("Read from file:");
        while (file.available()) {
            Serial.write(file.read());
        }
        file.close();
    }

    void writeFile(fs::FS &fs, const char *path, const char *message) {
        log_i("Writing file: %s", path);

        File file = fs.open(path, FILE_WRITE);
        if (!file) {
            log_i("Failed to open file for writing");
            return;
        }
        if (file.print(message)) {
            log_i("File written");
        } else {
            log_i("Write failed");
        }
        file.close();
    }

    void appendFile(fs::FS &fs, const char *path, const char *message) {
        log_i("Appending to file: %s", path);

        File file = fs.open(path, FILE_APPEND);
        if (!file) {
            log_i("Failed to open file for appending");
            return;
        }
        if (file.print(message)) {
            log_i("Message appended");
        } else {
            log_i("Append failed");
        }
        file.close();
    }

    void renameFile(fs::FS &fs, const char *path1, const char *path2) {
        log_i("Renaming file %s to %s", path1, path2);
        if (fs.rename(path1, path2)) {
            log_i("File renamed");
        } else {
            log_i("Rename failed");
        }
    }

    void deleteFile(fs::FS &fs, const char *path) {
        log_i("Deleting file: %s", path);
        if (fs.remove(path)) {
            log_i("File deleted");
        } else {
            log_i("Delete failed");
        }
    }

    void testFileIO(fs::FS &fs, const char *path) {
        static uint8_t buf[512];
        size_t len = 0;
        ulong start = millis();

        File file = fs.open(path, FILE_WRITE);
        if (!file) {
            log_i("Failed to open file for writing");
            return;
        }
        for (size_t i = 0; i < 2048; i++) {
            file.write(buf, 512);
        }
        log_i("%u bytes written in %u ms", 2048 * 512, millis() - start);
        file.close();

        file = fs.open(path);
        if (!file) {
            log_i("Failed to open file for reading");
            return;
        }
        len = file.size();
        size_t flen = len;
        start = millis();
        while (len) {
            size_t toRead = len;
            if (toRead > 512) {
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        log_i("%u bytes read in %u ms", flen, millis() - start);
        file.close();
    }
};

#endif