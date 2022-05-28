#ifndef __webserver_h
#define __webserver_h

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "atoll_fs.h"
#include "recorder.h"
#include "atoll_ota.h"

#ifndef WEBSERVER_FS_TASK_STACK
#define WEBSERVER_FS_TASK_STACK 8192
#endif

class Webserver {
    typedef AsyncWebServerRequest Request;
    typedef AsyncWebServerResponse Response;

   public:
    enum FsTaskType {
        NO_TASK,
        INDEX_GENERATOR_TASK,
        REMOVE_ALL_TASK
        //, GPX_GENERATOR_TASK
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
        // if (!fs->mounted) {
        //     log_e("fs not mounted");
        //     return;
        // }
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
        // server->on("/gpxregen", HTTP_GET, [this](Request *request) {
        //     onGpxRegen(request);
        // });
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
    }

    void start();
    void stop();

    void onIndex(Request *request) {
        log_i("request %s", request->url().c_str());

        if (nullptr == fs) {
            log_e("fs is null");
            request->send(500, "text/html", "fs error");
            return;
        }
        if (!atollFs->aquireMutex()) {
            log_e("could not aquire mutex");
            return;
        }
        if (nullptr == recorder) {
            log_e("recorder is null");
            request->send(500, "text/html", "rec error");
            atollFs->releaseMutex();
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
            atollFs->releaseMutex();
            return;
        }
        log_i("sending %s", indexPath);
        request->send(*fs, indexPath, "text/html; charset=iso8859-1");
        atollFs->releaseMutex();
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
        if (!atollFs->aquireMutex()) {
            log_e("could not aquire mutex");
            return;
        }
        if (nullptr == recorder) {
            log_e("recorder is null");
            request->send(500, "text/html", "rec error");
            atollFs->releaseMutex();
            return;
        }
        char path[strlen(recorder->basePath) + 14] = "";
        if (!pathFromRequest(request, path, sizeof(path), ".gpx")) {
            log_i("could not get path");
            request->send(404, "text/html", "not found");
            atollFs->releaseMutex();
            return;
        }
        if (!fs->exists(path)) {
            log_i("%s does not extist", path);
            request->send(404, "text/html", "not found");
            atollFs->releaseMutex();
            return;
        }
        log_i("path: %s", path);
        if (nullptr != ota) {
            log_i("calling ota->stop()");
            ota->stop();  // ota hangs esp networking on long download, TODO restart after done
        }
        // request->send(*fs, path, "application/gpx+xml", true);

        File file = fs->open(path);
        if (!file) {
            log_i("could not get file");
            request->send(404, "text/html", "not found");
            atollFs->releaseMutex();
            return;
        }
        size_t fileSize = file.size();
        char fileName[20] = "";
        strncpy(fileName, file.name(), 20);
        file.close();
        atollFs->releaseMutex();
        Atoll::Fs *atollFsp = atollFs;
        fs::FS *fsp = fs;
        char fixedPath[32] = "";
        strncpy(fixedPath, path, 32);
        Response *response = request->beginResponse(
            "application/gpx+xml", fileSize,
            [atollFsp, fsp, fixedPath](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
                if (!atollFsp->aquireMutex()) {
                    log_e("could not aquire mutex");
                    return 0;
                }
                if (!fsp || !fsp->exists(fixedPath)) {
                    log_e("cannot find %s", fixedPath);
                    return 0;
                }
                File file = fsp->open(fixedPath);
                if (!file) {
                    log_e("cannot open %s", fixedPath);
                    return 0;
                }
                if (!file.seek(index)) {
                    file.close();
                    log_e("cannot seek to %d", index);
                    return 0;
                }
                size_t bytes = file.read(buffer, maxLen);
                // log_d("%s read %d (max %d), index %d", file.path(), bytes, maxLen, index);
                if (bytes + index == file.size())
                    log_i("sent %s %d bytes", fixedPath, bytes + index);
                file.close();
                atollFsp->releaseMutex();
                return bytes;
            });
        char buf[64];
        snprintf(buf, sizeof(buf), "attachment; filename=\"%s\"", fileName);
        response->addHeader("Content-Disposition", buf);
        request->send(response);
    }

    void onRemove(Request *request) {
        log_i("request %s", request->url().c_str());

        if (nullptr == fs) {
            log_e("fs is null");
            request->send(500, "text/html", "fs error");
            return;
        }
        if (!atollFs->aquireMutex()) {
            log_e("could not aquire mutex");
            return;
        }
        if (nullptr == recorder) {
            log_e("recorder is null");
            request->send(500, "text/html", "rec error");
            atollFs->releaseMutex();
            return;
        }
        char path[strlen(recorder->basePath) + 14] = "";
        if (!pathFromRequest(request, path, sizeof(path))) {
            log_i("could not get path");
            request->send(404, "text/html", "not found");
            atollFs->releaseMutex();
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
        atollFs->releaseMutex();
        Response *response = request->beginResponse(302, "text/plain", "Moved");
        response->addHeader("Location", "/");
        request->send(response);
        generateIndex();
    }

    // void onGpxRegen(Request *request)
    //     log_i("request %s", request->url().c_str());
    //     if (nullptr == fs) {
    //         log_e("fs is null");
    //         request->send(500, "text/html", "fs error");
    //         return;
    //     }
    //     if (nullptr == recorder) {
    //         log_e("recorder is null");
    //         request->send(500, "text/html", "rec error");
    //         return;
    //     }
    //     char path[strlen(recorder->basePath) + 14] = "";
    //     if (!pathFromRequest(request, path, sizeof(path))) {
    //         log_i("could not get path");
    //         request->send(404, "text/html", "not found");
    //         return;
    //     }
    //     if (!fs->exists(path)) {
    //         log_i("%s not found", path);
    //         request->send(404, "text/html", "not found");
    //     }
    //     char gpxPath[strlen(path) + 5];
    //     snprintf(gpxPath, sizeof(gpxPath), "%s.gpx", path);
    //     if (fs->exists(gpxPath))
    //         log_i("%s %s", fs->remove(gpxPath) ? "removed" : "could not remove", gpxPath);
    //     runFsTask(FsTaskType::GPX_GENERATOR_TASK);
    //     Response *response = request->beginResponse(302, "text/plain", "Moved");
    //     response->addHeader("Location", "/");
    //     request->send(response);
    // }

    void onRemoveAll(Request *request) {
        genIndexResponse(request);
        removeAll();
        // delay(500);
        // while (fsTaskRunning) {
        //     delay(500);
        //     yield();
        // }
        // generateIndex();
    }

    void onFormat(Request *request) {
        request->send(200, "text/html", "not supported");
        // genIndexResponse(request);
        //  atollFs->unmount();
        // atollFs->format();
        //  atollFs->setup();
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
            // case FsTaskType::GPX_GENERATOR_TASK:
            //     task = gpxGeneratorTask;
            //     break;
            case FsTaskType::REMOVE_ALL_TASK:
                task = removeAllTask;
                break;
            default:
                log_e("unknown task type %d", taskType);
                return;
        }
        xTaskCreatePinnedToCore(
            task,
            "Webserver FsTask",
            WEBSERVER_FS_TASK_STACK,
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
        Webserver *thisP = (Webserver *)p;
        thisP->indexGeneratorRunner();
        thisP->fsTaskStop();
    }

    void indexGeneratorRunner() {
        log_i("starting run");
        fsTaskRunning = FsTaskType::INDEX_GENERATOR_TASK;
        if (nullptr == fs) {
            log_e("fs is null");
            return;
        }
        if (!atollFs->aquireMutex()) {
            log_e("could not aquire mutex");
            return;
        }
        if (nullptr == recorder) {
            log_e("recorder is null");
            atollFs->releaseMutex();
            return;
        }
        File dir = fs->open(recorder->basePath);
        if (!dir) {
            log_e("could not open %s", recorder->basePath);
            dir.close();
            atollFs->releaseMutex();
            return;
        }
        if (!dir.isDirectory()) {
            log_e("%s is not a directory", recorder->basePath);
            dir.close();
            atollFs->releaseMutex();
            return;
        }
        char indexPath[strlen(recorder->basePath) + strlen(indexFileName) + 2] = "";
        snprintf(indexPath, sizeof(indexPath), "%s/%s", recorder->basePath, indexFileName);
        File index = fs->open(indexPath, FILE_WRITE);
        if (!index) {
            log_e("could not open %s for writing", indexPath);
            dir.close();
            atollFs->releaseMutex();
            return;
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
        <p><a href="/genIndex">regenerate index</a></p>
        <p><a href="/removeAll">delete all</a></p>
        <!--<p><a href="/format">format</a></p>-->
    </body>
</html>
)====";

        if (strlen(header) != index.write((uint8_t *)header, strlen(header))) {
            log_e("could not write header to %s", indexPath);
            index.close();
            dir.close();
            atollFs->releaseMutex();
            return;
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
            // file.name() used to return the path, now it returns the file name
            strncpy(filePath, file.path(), sizeof(filePath));
            file.close();
            log_i("found %s", filePath);

            char id[strlen(filePath) - basePathLen - 5] = "";
            strncat(id, filePath + basePathLen + 1, sizeof(id));
            // char id[strlen(filePath) - 4] = "";
            // strncat(id, filePath, sizeof(id));
            log_i("id: %s", id);
            char stxPath[basePathLen + strlen(id) + 6] = "";
            snprintf(stxPath, sizeof(stxPath), "%s/%s.stx",
                     recorder->basePath, id);
            Recorder::Stats stats;
            if (fs->exists(stxPath)) {
                File stx = fs->open(stxPath);
                stx.read((uint8_t *)&stats, sizeof(stats));
                stx.close();
            } else
                log_w("missing %s", stxPath);
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
        atollFs->releaseMutex();
    }

    static void removeAllTask(void *p) {
        Webserver *thisP = (Webserver *)p;
        thisP->removeAllRunner();
        thisP->indexGeneratorRunner();
        thisP->fsTaskStop();
    }

    void removeAllRunner() {
        log_i("starting run");
        fsTaskRunning = FsTaskType::REMOVE_ALL_TASK;
        if (nullptr == fs) {
            log_e("fs is null");
            return;
        }
        if (!atollFs->aquireMutex()) {
            log_e("could not aquire mutex");
            return;
        }
        if (nullptr == recorder) {
            log_e("recorder is null");
            atollFs->releaseMutex();
            return;
        }
        File dir = fs->open(recorder->basePath);
        if (!dir) {
            log_e("cannot open %s", recorder->basePath);
            atollFs->releaseMutex();
            return;
        }
        File file;
        uint8_t pathLen = ATOLL_RECORDER_PATH_LENGTH;
        char path[pathLen] = "";
        char contPath[pathLen] = "";
        if (fs->exists(recorder->continuePath) &&
            (file = fs->open(recorder->continuePath))) {
            file.read((uint8_t *)contPath, pathLen);
            file.close();
        }
        char contStatsPath[pathLen] = "";
        if (strlen(contPath)) {
            strncpy(contStatsPath, contPath, pathLen);
            recorder->appendStatsExt(contStatsPath, pathLen);
        }
        const char *currPath = recorder->currentPath();
        char currStatsPath[pathLen] = "";
        if (nullptr != currPath) {
            strncpy(currStatsPath, currPath, pathLen);
            recorder->appendStatsExt(currStatsPath, pathLen);
        }
        while (file = dir.openNextFile()) {
            if (file.isDirectory()) {
                file.close();
                continue;
            }
            snprintf(path, pathLen, "%s", file.path());
            file.close();
            if (0 == strcmp(path, contPath) ||
                0 == strcmp(path, contStatsPath) ||
                0 == strcmp(path, recorder->continuePath) ||
                (nullptr != currPath && 0 == strcmp(path, currPath)) ||
                0 == strcmp(path, currStatsPath)) {
                log_i("skipping %s", path);
                delay(100);  // for wifiserial
                continue;
            }
            log_i("%s %s", fs->remove(path) ? "deleted" : "could not delete", path);
            // log_i("delete %s", path);
            delay(100);  // for wifiserial
        }
        dir.close();
        atollFs->releaseMutex();
    }

    bool pathFromRequest(Request *request, char *path, size_t len, const char *ext = "") {
        if (!request->hasParam("f")) {
            log_e("no param");
            return false;
        }
        const char *f = request->getParam("f")->value().c_str();
        // log_i("f: %s", f);
        if (strlen(f) < 1) {
            log_e("param is empty");
            return false;
        }
        if (strchr(f, '/')) {
            log_e("'/' in param '%s'", f);
            return false;
        }
        snprintf(path, len, "%s/%s%s",
                 recorder->basePath,
                 f,
                 ext);
        if (nullptr == fs) {
            log_e("fs is null");
            return false;
        }
        if (!atollFs->aquireMutex()) {
            log_e("could not aquire mutex");
            return false;
        }
        if (!fs->exists(path)) {
            log_i("%s does not exist", path);
        }
        atollFs->releaseMutex();
        return true;
    }
};

#endif