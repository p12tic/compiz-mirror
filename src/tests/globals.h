#ifndef GLOBALS_H
#define GLOBALS_H

char *programName;
char **programArgv;
int  programArgc;

bool shutDown = false;
bool restartSignal = false;

CompWindow *lastFoundWindow = 0;

bool replaceCurrentWm = false;
bool indirectRendering = false;
bool noDetection = false;
bool useDesktopHints = false;
bool debugOutput = false;
bool useCow = true;

std::list <CompString> initialPlugins;

unsigned int pluginClassHandlerIndex = 0;

#endif // GLOBALS_H
