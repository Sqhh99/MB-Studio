// MB-Studio project plugin - processing requests as JSON, so that the
// project history can run a job again (retry) and remake a product from its
// recorded recipe (regenerate), also in a later session.
#pragma once

#include <QJsonObject>

#include "mbstudio/api/ProjectService.h"

namespace mbstudio::project {

QJsonObject requestToJson(const ProcessingRequest &request);
// Fields missing in the object keep their value in request.
bool requestFromJson(const QJsonObject &object, ProcessingRequest &request);

}  // namespace mbstudio::project
