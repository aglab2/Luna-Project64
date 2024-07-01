#pragma once

#include <Project64-core/Notification.h>
#include <Project64-core/CoreApi.h>

COREAPI bool AppInit(CNotification * Notify, const char * BaseDirectory, int argc, char **argv);
COREAPI void AppCleanup(void);
