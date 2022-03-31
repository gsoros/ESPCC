#ifndef __rec_webserver_h
#define __rec_webserver_h

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "atoll_fs.h"
#include "atoll_recorder.h"
#include "atoll_ota.h"

class RecWebserver {
    typedef AsyncWebServerRequest Req;
    typedef AsyncWebServerResponse Resp;

   public:
    FS *fs = nullptr;                             //
    AsyncWebServer *server = nullptr;             //
    Atoll::Recorder *recorder = nullptr;          //
    Atoll::Ota *ota = nullptr;                    //
    const char *indexFileName = "index.html";     //
    TaskHandle_t generateIndexTaskHandle = NULL;  //
    bool generatingIndex = false;                 // TODO checking TaskHandle == NULL does not work reliably

    void setup(
        Atoll::Fs *fs,
        Atoll::Recorder *recorder,
        Atoll::Ota *ota) {
        if (nullptr == fs) {
            log_e("fs is null");
            return;
        }
        if (!fs->mounted) {
            log_e("fs not mounted");
            return;
        }
        this->fs = fs->pFs();

        if (nullptr == recorder) {
            log_e("recorder is null");
            return;
        }
        this->recorder = recorder;
        this->ota = ota;

        server = new AsyncWebServer(80);
        server->on("/", HTTP_GET, [this](Req *request) {
            list(request);
        });
        server->on("/download", HTTP_GET, [this](Req *request) {
            download(request);
        });
        server->on("/remove", HTTP_GET, [this](Req *request) {
            remove(request);
        });
        server->on("/genIndex", HTTP_GET, [this](Req *request) {
            genIndex(request);
        });
        server->onNotFound([](Req *request) {
            if (nullptr == strstr("favicon.ico", request->url().c_str())) {
                request->send(404, "text/plain", "404 Not found");
                return;
            }
            log_i("404 request %s", request->url().c_str());
            Resp *response = request->beginResponse(302, "text/plain", "Moved");
            response->addHeader("Location", "/");
            request->send(response);
        });
        server->begin();
        log_i("serving on port 80");
    }

    void list(Req *request) {
        log_i("request %s", request->url().c_str());

        if (nullptr == fs) {
            log_e("fs is null");
            request->send(500, "text/html", "fs error");
            return;
        }
        if (nullptr == recorder) {
            log_e("recorder is null");
            request->send(500, "text/html", "rec error");
            return;
        }
        char indexPath[strlen(recorder->basePath) + strlen(indexFileName) + 2] = "";
        snprintf(indexPath, sizeof(indexPath), "%s/%s", recorder->basePath, indexFileName);
        log_i("checking %s", indexPath);
        bool indexExists = fs->exists(indexPath);
        if (!indexExists || generatingIndex) {
            if (!indexExists)
                log_i("%s does not exist", indexPath);
            if (!generatingIndex) {
                log_i("calling generator");
                generateIndex();
            } else
                log_i("generator is running");
            genIndexResponse(request);
            return;
        }
        log_i("sending %s", indexPath);
        request->send(*fs, indexPath, "text/html; charset=iso8859-1");
    }

    void genIndex(Req *request) {
        genIndexResponse(request);
        generateIndex();
    }

    void genIndexResponse(Req *request) {
        request->send(200, "text/html", R"====(<!DOCTYPE html><html><body>
        <p>Generating index...</p><a href="/">reload</a></body></html>)====");
    }

    void generateIndex() {
        if (generatingIndex) {
            log_i("already generating");
            return;
        }
        log_i("starting task");
        xTaskCreatePinnedToCore(
            generateIndexTask,
            "Generate Index",
            4096,
            this,
            1,
            &generateIndexTaskHandle,
            1);
    }

    static void generateIndexTask(void *p) {
        RecWebserver *thisP = (RecWebserver *)p;
        thisP->generateIndexRunner();
        thisP->generateIndexTaskStop();
    }

    void generateIndexTaskStop() {
        generatingIndex = false;
        if (NULL != generateIndexTaskHandle) {
            log_i("deleting task");
            vTaskDelete(generateIndexTaskHandle);
        }
    }

    void generateIndexRunner() {
        log_i("starting run");
        generatingIndex = true;
        if (nullptr == fs) {
            log_e("fs is null");
            generateIndexTaskStop();
        }
        if (nullptr == recorder) {
            log_e("recorder is null");
            generateIndexTaskStop();
        }
        File dir = fs->open(recorder->basePath);
        if (!dir) {
            log_e("could not open %s", recorder->basePath);
            dir.close();
            generateIndexTaskStop();
        }
        if (!dir.isDirectory()) {
            log_e("%s is not a directory", recorder->basePath);
            dir.close();
            generateIndexTaskStop();
        }
        char indexPath[strlen(recorder->basePath) + strlen(indexFileName) + 2] = "";
        snprintf(indexPath, sizeof(indexPath), "%s/%s", recorder->basePath, indexFileName);
        File index = fs->open(indexPath, FILE_WRITE);
        if (!index) {
            log_e("could not open %s for writing", indexPath);
            dir.close();
            generateIndexTaskStop();
        }

        char header[1024] = R"====(<!DOCTYPE html>
<html>
    <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
    </head>
    <body>
        <table>
            <tr>
            <td></td>
            <td>distance</td>
            <td>alt gain</td>
            <td>remove</td>
            </tr>)====";

        const char *itemFormat = R"====(
            <tr>
            <td><a href="download?f=%s">%s</a></td>
            <td>%.0fm</td>
            <td>%dm</td>
            <td><a href="remove?f=%s">remove</a></td>
            </tr>)====";

        const char *footer = R"====(
        </table>
    </body>
</html>
)====";

        if (strlen(header) != index.write((uint8_t *)header, strlen(header))) {
            log_e("could not write header to %s", indexPath);
            index.close();
            dir.close();
            generateIndexTaskStop();
        }
        File file;
        char filePath[ATOLL_RECORDER_PATH_LENGTH] = "";
        uint8_t basePathLen = strlen(recorder->basePath);
        uint16_t files = 0;
        char ext[5] = "";
        while (file = dir.openNextFile()) {
            snprintf(ext, sizeof(ext), "%s", file.name() + strlen(file.name()) - 4);
            // log_i("%s ext is %s", file.name(), ext);
            if (0 != strcmp(ext, ".gpx")) {
                // log_i("%s is not a gpx file, skipping", file.name());
                file.close();
                continue;
            }
            strncpy(filePath, file.name(), sizeof(filePath));
            file.close();
            log_i("found %s", filePath);
            char id[strlen(filePath) - basePathLen - 5] = "";
            strncat(id, filePath + basePathLen + 1, sizeof(id));
            // log_i("id: %s", id);
            char stxPath[basePathLen + strlen(id) + 6] = "";
            snprintf(stxPath, sizeof(stxPath), "%s/%s.stx",
                     recorder->basePath, id);
            Recorder::Stats stats;
            if (fs->exists(stxPath)) {
                File stx = fs->open(stxPath);
                stx.read((uint8_t *)&stats, sizeof(stats));
                stx.close();
            }
            char item[strlen(itemFormat)  //
                      + sizeof(id) * 3    //
                      + 10                // distance
                      + 5                 // altGain
            ];
            snprintf(item, sizeof(item), itemFormat,
                     id,
                     id,
                     stats.distance,
                     stats.altGain,
                     id);
            // log_i("item: %s", item);
            if (strlen(item) != index.write((uint8_t *)item, strlen(item))) {
                log_e("could not write item to %s", indexPath);
                continue;
            }
            files++;
        }
        dir.close();
        if (0 == files) {
            const char *nr = "No files";
            index.write((uint8_t *)nr, strlen(nr));
        }
        if (strlen(footer) != index.write((uint8_t *)footer, strlen(footer)))
            log_e("could not write footer to %s", indexPath);
        index.close();  // re-open to get size
        index = fs->open(indexPath);
        log_i("generated %s %d bytes", indexPath, index.size());
        index.close();
        generateIndexTaskStop();
    }

    void download(Req *request) {
        log_i("request %s", request->url().c_str());

        if (nullptr == fs) {
            log_e("fs is null");
            request->send(500, "text/html", "fs error");
            return;
        }
        if (nullptr == recorder) {
            log_e("recorder is null");
            request->send(500, "text/html", "rec error");
            return;
        }
        char path[strlen(recorder->basePath) + 14] = "";
        if (!pathFromRequest(request, path, sizeof(path), ".gpx")) {
            log_i("could not get path");
            request->send(404, "text/html", "not found");
            return;
        }
        if (nullptr != ota)
            ota->off();  // ota crashes esp on long download, TODO restart after done
        request->send(*fs, path, "application/gpx+xml", true);
    }

    void remove(Req *request) {
        log_i("request %s", request->url().c_str());

        if (nullptr == fs) {
            log_e("fs is null");
            request->send(500, "text/html", "fs error");
            return;
        }
        if (nullptr == recorder) {
            log_e("recorder is null");
            request->send(500, "text/html", "rec error");
            return;
        }
        char path[strlen(recorder->basePath) + 14] = "";
        if (!pathFromRequest(request, path, sizeof(path))) {
            log_i("could not get path");
            request->send(404, "text/html", "not found");
            return;
        }
        if (fs->exists(path))
            log_i("%s %s", fs->remove(path) ? "removed" : "could not remove", path);
        char extPath[strlen(path) + 5];
        snprintf(extPath, sizeof(extPath), "%s.gpx", path);
        if (fs->exists(extPath))
            log_i("%s %s", fs->remove(extPath) ? "removed" : "could not remove", extPath);
        snprintf(extPath, sizeof(extPath), "%s.stx", path);
        if (fs->exists(extPath))
            log_i("%s %s", fs->remove(extPath) ? "removed" : "could not remove", extPath);
        Resp *response = request->beginResponse(302, "text/plain", "Moved");
        response->addHeader("Location", "/");
        request->send(response);
        generateIndex();
    }

    bool pathFromRequest(Req *request, char *path, size_t len, const char *ext = "") {
        if (!request->hasParam("f")) {
            log_e("no param");
            return false;
        }
        const char *f = request->getParam("f")->value().c_str();
        if (strchr(f, '/')) {
            log_e("slash in param '%s'", f);
            return false;
        }
        snprintf(path, len, "%s/%s%s",
                 recorder->basePath,
                 f,
                 ext);
        if (!fs->exists(path)) {
            log_i("%s does not exist", path);
            // return false;
        }
        return true;
    }
};

#endif