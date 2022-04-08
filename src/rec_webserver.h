#ifndef __rec_webserver_h
#define __rec_webserver_h

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "atoll_fs.h"
#include "recorder.h"
#include "atoll_ota.h"

class RecWebserver {
    typedef AsyncWebServerRequest Request;
    typedef AsyncWebServerResponse Response;

   public:
    enum FsTaskType {
        NO_TASK,
        INDEX_GENERATOR_TASK,
        REMOVE_ALL_TASK
    };
    FS *fs = nullptr;                             //
    Atoll::Fs *atollFs = nullptr;                 //
    AsyncWebServer *server = nullptr;             //
    Recorder *recorder = nullptr;                 //
    Atoll::Ota *ota = nullptr;                    //
    const char *indexFileName = "index.html";     //
    TaskHandle_t fsTaskHandle = NULL;             //
    uint8_t fsTaskRunning = FsTaskType::NO_TASK;  //
    uint16_t port = 80;                           //
    bool serving = false;                         //

    void setup(
        Atoll::Fs *fs,
        Recorder *recorder,
        Atoll::Ota *ota) {
        if (nullptr == fs) {
            log_e("fs is null");
            return;
        }
        if (!fs->mounted) {
            log_e("fs not mounted");
            return;
        }
        this->atollFs = fs;
        this->fs = fs->pFs();

        if (nullptr == recorder) {
            log_e("recorder is null");
            return;
        }
        this->recorder = recorder;
        this->ota = ota;

        server = new AsyncWebServer(port);
        server->on("/", HTTP_GET, [this](Request *request) {
            onIndex(request);
        });
        server->on("/download", HTTP_GET, [this](Request *request) {
            onDownload(request);
        });
        server->on("/remove", HTTP_GET, [this](Request *request) {
            onRemove(request);
        });
        server->on("/removeAll", HTTP_GET, [this](Request *request) {
            onRemoveAll(request);
        });
        server->on("/format", HTTP_GET, [this](Request *request) {
            onFormat(request);
        });
        server->on("/genIndex", HTTP_GET, [this](Request *request) {
            onGenIndex(request);
        });
        server->onNotFound([](Request *request) {
            if (nullptr != strstr(request->url().c_str(), "favicon.ico")) {
                request->send(404, "text/plain", "404");
                // log_i("404 %s", request->url().c_str());
                return;
            }
            Response *response = request->beginResponse(302, "text/plain", "Moved");
            response->addHeader("Location", "/");
            request->send(response);
            log_i("302 %s", request->url().c_str());
        });
        begin();
    }

    void begin();
    void end();

    void onIndex(Request *request) {
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
        if (!indexExists || fsTaskRunning) {
            if (!indexExists)
                log_i("%s does not exist", indexPath);
            if (!fsTaskRunning) {
                log_i("calling generator");
                generateIndex();
            } else
                log_i("fs task is running");
            genIndexResponse(request);
            return;
        }
        log_i("sending %s", indexPath);
        request->send(*fs, indexPath, "text/html; charset=iso8859-1");
    }

    void onGenIndex(Request *request) {
        genIndexResponse(request);
        generateIndex();
    }

    void onDownload(Request *request) {
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

    void onRemove(Request *request) {
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
        Response *response = request->beginResponse(302, "text/plain", "Moved");
        response->addHeader("Location", "/");
        request->send(response);
        generateIndex();
    }

    void onRemoveAll(Request *request) {
        genIndexResponse(request);
        removeAll();
        delay(500);
        while (fsTaskRunning) delay(500);
        generateIndex();
    }

    void onFormat(Request *request) {
        genIndexResponse(request);
        // atollFs->unmount();
        // atollFs->format();
        // atollFs->setup();
        // delay(500);
        // generateIndex();
    }

    void genIndexResponse(Request *request) {
        request->send(200, "text/html", R"====(<!DOCTYPE html><html><body>
        <p>Generating index...</p><a href="/">reload</a></body></html>)====");
    }

    void generateIndex() {
        runFsTask(FsTaskType::INDEX_GENERATOR_TASK);
    }

    void removeAll() {
        runFsTask(FsTaskType::REMOVE_ALL_TASK);
    }

    void runFsTask(uint8_t taskType) {
        if (fsTaskRunning) {
            log_i("task is already running: %d", fsTaskRunning);
            return;
        }
        log_i("starting task %d", taskType);
        TaskFunction_t task;
        switch (taskType) {
            case FsTaskType::INDEX_GENERATOR_TASK:
                task = indexGeneratorTask;
                break;
            case FsTaskType::REMOVE_ALL_TASK:
                task = removeAllTask;
                break;
            default:
                log_e("unknown task type %d", taskType);
                return;
        }
        xTaskCreatePinnedToCore(
            task,
            "RecWebserver FsTask",
            4096,
            this,
            1,
            &fsTaskHandle,
            1);
    }

    void fsTaskStop() {
        fsTaskRunning = FsTaskType::NO_TASK;
        if (NULL != fsTaskHandle) {
            log_i("deleting task");
            vTaskDelete(fsTaskHandle);
        }
    }

    static void indexGeneratorTask(void *p) {
        RecWebserver *thisP = (RecWebserver *)p;
        thisP->indexGeneratorRunner();
        thisP->fsTaskStop();
    }

    void indexGeneratorRunner() {
        log_i("starting run");
        fsTaskRunning = FsTaskType::INDEX_GENERATOR_TASK;
        if (nullptr == fs) {
            log_e("fs is null");
            fsTaskStop();
        }
        if (nullptr == recorder) {
            log_e("recorder is null");
            fsTaskStop();
        }
        File dir = fs->open(recorder->basePath);
        if (!dir) {
            log_e("could not open %s", recorder->basePath);
            dir.close();
            fsTaskStop();
        }
        if (!dir.isDirectory()) {
            log_e("%s is not a directory", recorder->basePath);
            dir.close();
            fsTaskStop();
        }
        char indexPath[strlen(recorder->basePath) + strlen(indexFileName) + 2] = "";
        snprintf(indexPath, sizeof(indexPath), "%s/%s", recorder->basePath, indexFileName);
        File index = fs->open(indexPath, FILE_WRITE);
        if (!index) {
            log_e("could not open %s for writing", indexPath);
            dir.close();
            fsTaskStop();
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
            <td>Distance</td>
            <td>Alt. gain</td>
            <td></td>
            </tr>)====";

        const char *itemFormat = R"====(
            <tr>
            <td><a href="download?f=%s">%s</a></td>
            <td>%.0fm</td>
            <td>%dm</td>
            <td><a href="remove?f=%s">delete</a></td>
            </tr>)====";

        const char *footer = R"====(
        </table>
        <p><a href="/genIndex">Regenerate index</a></p>
        <p><a href="/removeAll">Delete all</a></p>
        <!--<p><a href="/format">Format SD</a></p>-->
    </body>
</html>
)====";

        if (strlen(header) != index.write((uint8_t *)header, strlen(header))) {
            log_e("could not write header to %s", indexPath);
            index.close();
            dir.close();
            fsTaskStop();
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
            const char *nr = "<tr><td colspan=4>No files</td></tr>";
            index.write((uint8_t *)nr, strlen(nr));
        }
        if (strlen(footer) != index.write((uint8_t *)footer, strlen(footer)))
            log_e("could not write footer to %s", indexPath);
        index.close();  // re-open to get size
        index = fs->open(indexPath);
        log_i("generated %s %d bytes", indexPath, index.size());
        index.close();
        fsTaskStop();
    }

    static void removeAllTask(void *p) {
        RecWebserver *thisP = (RecWebserver *)p;
        thisP->removeAllRunner();
        thisP->fsTaskStop();
    }

    void removeAllRunner() {
        log_i("starting run");
        fsTaskRunning = FsTaskType::REMOVE_ALL_TASK;
        if (nullptr == fs) {
            log_e("fs is null");
            fsTaskStop();
        }
        if (nullptr == recorder) {
            log_e("recorder is null");
            fsTaskStop();
        }
        File dir = fs->open(recorder->basePath);
        if (!dir) {
            log_e("cannot open %s", recorder->basePath);
            fsTaskStop();
        }
        File file;
        char path[ATOLL_RECORDER_PATH_LENGTH] = "";
        char cont[sizeof(path)] = "";
        if (file = fs->open(recorder->continuePath)) {
            file.read((uint8_t *)cont, sizeof(cont));
            file.close();
        }
        const char *cp = recorder->currentPath();
        const char *csp = recorder->currentStatsPath();
        while (file = dir.openNextFile()) {
            if (file.isDirectory()) {
                file.close();
                continue;
            }
            snprintf(path, sizeof(path), "%s", file.name());
            file.close();
            if (0 == strcmp(path, cont) ||
                0 == strcmp(path, recorder->continuePath) ||
                (nullptr != cp && 0 == strcmp(path, cp)) ||
                (nullptr != csp && 0 == strcmp(path, csp))) {
                log_i("skipping %s", path);
                continue;
            }
            log_i("%s %s", fs->remove(path) ? "deleted" : "could not delete", path);
        }
        dir.close();
        fsTaskStop();
    }

    bool pathFromRequest(Request *request, char *path, size_t len, const char *ext = "") {
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