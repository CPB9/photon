#ifdef PHOTON_STUB

#include "photongen/onboard/asv/Asv.Component.h"
#include "photongen/onboard/Autosave.inc.c"

#include <stdio.h>
#include <stdlib.h>

static const char* varPath = "photon-model-vars-" PHOTON_DEVICE_NAME ".pasv";

PhotonError PhotonAsv_SaveVars()
{
    void* data = malloc(_PHOTON_AUTOSAVE_MAX_SIZE);
    if (!data) {
        return PhotonError_NotEnoughSpace;
    }
    PhotonWriter dest;
    PhotonWriter_Init(&dest, data, _PHOTON_AUTOSAVE_MAX_SIZE);

    PhotonError rv = Photon_SerializeParams(&dest);
    if (rv != PhotonError_Ok) {
        free(data);
        return rv;
    }

    FILE* file = fopen(varPath, "wb");
    if (!file) {
        free(data);
        return PhotonError_NotEnoughSpace;
    }

    size_t dataSize = dest.current - dest.start;
    size_t writtenSize = fwrite(data, 1, dataSize, file);
    if (writtenSize != dataSize) {
        fclose(file);
        free(data);
        return PhotonError_NotEnoughSpace;
    }

    fclose(file);
    free(data);
    return PhotonError_Ok;
}

PhotonError PhotonAsv_LoadVars()
{
    FILE* file = fopen(varPath, "rb");
    if (!file) {
        return PhotonError_NotEnoughData;
    }

    if (fseek(file, 0, SEEK_END)) {
        fclose(file);
        return PhotonError_NotEnoughData;
    }

    long size = ftell(file);

    if (size == EOF) {
        fclose(file);
        return PhotonError_NotEnoughData;
    }

    if (fseek(file, 0, SEEK_SET)) {
        fclose(file);
        return PhotonError_NotEnoughData;
    }

    void* data = malloc(size);
    if (!data) {
        fclose(file);
        return PhotonError_NotEnoughData;
    }

    size_t bytesRead = fread(data, 1, size, file);
    if (bytesRead != (size_t)size) {
        free(data);
        fclose(file);
        return PhotonError_NotEnoughData;
    }

    PhotonReader reader;
    PhotonReader_Init(&reader, data, bytesRead);

    PhotonError rv = Photon_DeserializeParams(&reader);
    if (rv != PhotonError_Ok) {
        free(data);
        fclose(file);
        return rv;
    }

    free(data);
    fclose(file);
    return PhotonError_Ok;
}

PhotonError PhotonAsv_ExecCmd_SaveVars()
{
    return PhotonAsv_SaveVars();
}

PhotonError PhotonAsv_ExecCmd_LoadVars()
{
    return PhotonAsv_LoadVars();
}

#endif
