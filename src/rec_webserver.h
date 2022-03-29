#ifndef __rec_webserver_h
#define __rec_webserver_h

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "atoll_task.h"
#include "atoll_fs.h"
#include "atoll_recorder.h"
#include "atoll_ota.h"

class RecWebserver : public Atoll::Task {
    typedef AsyncWebServerRequest Req;
    typedef AsyncWebServerResponse Resp;

   public:
    const char *taskName() { return "RecWebserver"; }
    FS *fs = nullptr;                     //
    AsyncWebServer *server = nullptr;     //
    Atoll::Recorder *recorder = nullptr;  //
    Atoll::Ota *ota = nullptr;            //

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
        server->onNotFound([this](Req *request) {
            log_i("404 request %s", request->url().c_str());
            Resp *response = request->beginResponse(302, "text/plain", "Moved");
            response->addHeader("Location", "/");
            request->send(response);
        });
        server->begin();
        log_i("serving on port 80");
    }

    void loop() {
        taskStop();
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
        File dir = fs->open(recorder->basePath);
        if (!dir) {
            log_e("could not open %s", recorder->basePath);
            request->send(500, "text/html", "basepath error");
            dir.close();
            return;
        }
        if (!dir.isDirectory()) {
            log_e("%s is not a directory", recorder->basePath);
            request->send(500, "text/html", "basepath not dir error");
            dir.close();
            return;
        }

        char html[1024] = R"====(<!DOCTYPE html>
<html>
    <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
    </head>
    <body>)====";
        const char *itemFormat = R"====(
        <p><a href="download?f=%s">%s</a> <a href="remove?f=%s">remove</a></p>)====";
        const char *footer = R"====(
    </body>
</html>
)====";

        uint16_t footerLen = strlen(footer);
        File file;
        char filePath[ATOLL_RECORDER_PATH_LENGTH] = "";
        char gpxPath[ATOLL_RECORDER_PATH_LENGTH] = "";
        const char *currentRecPath = recorder->currentPath();
        uint8_t basePathLen = strlen(recorder->basePath);
        while (file = dir.openNextFile()) {
            if (strlen(file.name()) - strlen(recorder->basePath) - 1 != 8) {
                // log_i("%s is not a recording, skipping", file.name());
                file.close();
                continue;
            }
            if (currentRecPath && 0 == strcmp(currentRecPath, file.name())) {
                log_i("currently recording %s, skipping", file.name());
                file.close();
                continue;
            }
            if (0 == strcmp(recorder->continuePath, file.name())) {
                // log_i("skipping %s", file.name());
                file.close();
                continue;
            }
            strncpy(filePath, file.name(), sizeof(filePath));
            file.close();
            log_i("found a recording: %s", filePath);
            snprintf(gpxPath, sizeof(gpxPath), "%s.gpx", filePath);
            if (!fs->exists(gpxPath)) continue;
            log_i("gpx exists: %s", gpxPath);
            char gpxId[strlen(filePath) - basePathLen] = "";
            strncat(gpxId, filePath + basePathLen + 1, sizeof(gpxId));
            log_i("gpxId: %s", gpxId);
            char item[strlen(itemFormat)   //
                      + sizeof(gpxId) * 3  //
            ];
            if (sizeof(html) < strlen(html) + sizeof(item) + footerLen + 1) {
                log_e("could not add %s, buffer too small", gpxId);
                break;
            }
            snprintf(item, sizeof(item), itemFormat, gpxId, gpxId, gpxId);
            log_i("item: %s", item);
            strncat(html, item, sizeof(item));
        }
        dir.close();

        strncat(html, footer, footerLen);

        request->send(200, "text/html", html);
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
        if (!recPathFromRequest(request, path, sizeof(path))) {
            log_i("could not get path");
            request->send(404, "text/html", "not found");
            return;
        }
        if (nullptr != ota)
            ota->off();
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
        if (!recPathFromRequest(request, path, sizeof(path))) {
            log_i("could not get path");
            request->send(404, "text/html", "not found");
            return;
        }
        if (!fs->remove(path)) {
            log_e("could not remove %s", path);
            request->send(500, "text/html", "error deleting file");
            return;
        }
        char extPath[strlen(path) + 4];
        snprintf(extPath, sizeof(extPath), "%s.gpx", path);
        if (fs->exists(extPath) && !fs->remove(extPath))
            log_e("could not remove %s", extPath);
        snprintf(extPath, sizeof(extPath), "%s.stx", path);
        if (fs->exists(extPath) && !fs->remove(extPath))
            log_e("could not remove %s", extPath);
        Resp *response = request->beginResponse(302, "text/plain", "Moved");
        response->addHeader("Location", "/");
        request->send(response);
    }

    bool recPathFromRequest(Req *request, char *path, size_t len) {
        if (!request->hasParam("f")) {
            log_e("no param");
            return false;
        }
        const char *f = request->getParam("f")->value().c_str();
        if (strchr(f, '/')) {
            log_e("slash in param '%s'", f);
            return false;
        }
        snprintf(path, len, "%s/%s.gpx",
                 recorder->basePath,
                 f);
        if (!fs->exists(path)) {
            log_e("%s does not exist", path);
            return false;
        }
        return true;
    }
};

#endif