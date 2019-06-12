/*
 * Media.cpp
 *
 * This file contains all the routines for manipulating media, playing,
 * stopping etc.
 *
 * Copyright (c) Malcolm Smith 2001-2017.  No warranty is provided.
 */

#include "pch.h"

UINT wDeviceID;
DWORD dwIndex;

struct MCILink
{
    TCHAR szExt[16];
    TCHAR szDevice[32];
};

struct MCILink * lpLinks;
DWORD nLinks;
DWORD AllocatedLinks;


int 
MplayResetHandlers()
{
    lpLinks = (struct MCILink *)malloc(sizeof(struct MCILink)*10);
    nLinks = 0;
    AllocatedLinks = 10;
    return MPLAY_ERROR_SUCCESS;
}

int 
MplayAddHandler(LPCTSTR szExt, LPCTSTR szHandler)
{
    struct MCILink Temp;
    DWORD i;
    if (_tcslen(szExt) >= sizeof(Temp.szExt)/sizeof(TCHAR)) {
        return MPLAY_ERROR_OVERFLOW;
    }
    if (_tcslen(szHandler) >= sizeof(Temp.szDevice)/sizeof(TCHAR)) {
        return MPLAY_ERROR_OVERFLOW;
    }
    _tcscpy(Temp.szExt, szExt);
    _tcscpy(Temp.szDevice, szHandler);
    _tcsupr(Temp.szExt);

    for (i = 0; i < nLinks; i++) {
        if (!_tcscmp(Temp.szExt, lpLinks[i].szExt)) {
            memcpy(&lpLinks[i], &Temp, sizeof(struct MCILink));
            return MPLAY_ERROR_ALREADY_EXISTS;
        }
    }
    nLinks++;
    if (nLinks >= AllocatedLinks) {
        AllocatedLinks += 10;
        lpLinks = (struct MCILink *)realloc(lpLinks,
            (size_t)AllocatedLinks * sizeof(struct MCILink));
    }
    memcpy(&lpLinks[nLinks-1], &Temp, sizeof(struct MCILink));

    return MPLAY_ERROR_SUCCESS;
}

LPCTSTR
MplayGetHandler(LPCTSTR szExt)
{
    DWORD i;
    for (i = 0; i < nLinks; i++) {
        if (!_tcscmp(szExt, lpLinks[i].szExt)) {
            return lpLinks[i].szDevice;
        }
    }

    return NULL;
}

LPTSTR
MplayEnumHandlerExtensions(LPTSTR szBuffer, DWORD cchBufferLength, LPCTSTR szSep)
{
    DWORD i;
    DWORD cchBufferConsumed;
    DWORD cchElementLength;
    TCHAR szTemp[sizeof(lpLinks[0].szExt) / sizeof(lpLinks[0].szExt[0]) + 4];

    cchBufferConsumed = 0;
    _tcscpy(szBuffer, _T(""));

    for (i = 0; i < nLinks; i++)
    {
        _stprintf(szTemp, _T("%s*.%s"), szSep, lpLinks[i].szExt);
        _tcslwr(szTemp);
        cchElementLength = (DWORD)_tcslen(szTemp);
        if (cchBufferConsumed + cchElementLength >= cchBufferLength) {
            return szBuffer;
        }
        _tcscpy(&szBuffer[cchBufferConsumed], szTemp);
        cchBufferConsumed += cchElementLength;
    }
    return szBuffer;
}

int
MplaySetAudioPos(HWND hWndNotify)
{
    if (wDeviceID != 0) {
        MCI_SEEK_PARMS mciSeekParms;
        MCI_PLAY_PARMS mciPlayParms;
        DWORD dwPos;
        DWORD err;

        dwPos = MplayGetTrackerPos();

        mciSeekParms.dwTo = dwPos;
        err = mciSendCommand(wDeviceID,
            MCI_SEEK,
            MCI_WAIT|MCI_TO,
            (DWORD_PTR)&mciSeekParms);
        if (err != 0) {
            return MCI_ERROR_TO_MPLAY_ERROR(err);
        }

        mciPlayParms.dwCallback = (DWORD_PTR) hWndNotify;
        mciPlayParms.dwFrom = dwPos;
        err = mciSendCommand(wDeviceID,
            MCI_PLAY,
            MCI_NOTIFY|MCI_FROM,
            (DWORD_PTR)&mciPlayParms);
        if (err != 0) {
            return MCI_ERROR_TO_MPLAY_ERROR(err);
        }

        return MPLAY_ERROR_SUCCESS;
    }

    return MPLAY_ERROR_NO_MEDIA;
}

int
MplayPauseAudio(HWND hWnd)
{
    UNREFERENCED_PARAMETER(hWnd);

    if (wDeviceID != 0) {
        MCI_GENERIC_PARMS mciPauseParms;
        DWORD err;

        err = mciSendCommand(wDeviceID,
            MCI_PAUSE,
            MCI_WAIT,
            (DWORD_PTR)&mciPauseParms);
        if (err != 0) {
            return MCI_ERROR_TO_MPLAY_ERROR(err);
        }
        return MPLAY_ERROR_SUCCESS;
    }
    return MPLAY_ERROR_NO_MEDIA;
}

int
MplayResumeAudio(HWND hWnd)
{
    if (wDeviceID != 0) {
        MCI_GENERIC_PARMS mciResumeParms;
        MCI_PLAY_PARMS mciPlayParms;
        DWORD err;
        DWORD dwPos;

        dwPos = MplayGetTrackerPos();

        err = mciSendCommand(wDeviceID,
            MCI_RESUME,
            MCI_WAIT,
            (DWORD_PTR)&mciResumeParms);
        if (err != 0) {
            return MCI_ERROR_TO_MPLAY_ERROR(err);
        }

        //
        //  The above MCI_RESUME command really should be sufficient.
        //  However, we still see cases where a resume leaves the stream
        //  "playing", updating time etc, but without generating any
        //  audio.  We "fix" this by sending down a fresh play from a
        //  given offset.  In future, pause/resume may not be implemented
        //  using MCI at all because of this.
        //
        mciPlayParms.dwCallback = (DWORD_PTR) hWnd;
        mciPlayParms.dwFrom = dwPos;
        err = mciSendCommand(wDeviceID,
            MCI_PLAY,
            MCI_NOTIFY|MCI_FROM,
            (DWORD_PTR)&mciPlayParms);
        if (err != 0) {
            return MCI_ERROR_TO_MPLAY_ERROR(err);
        }

        return MPLAY_ERROR_SUCCESS;
    }
    return MPLAY_ERROR_NO_MEDIA;
}


int
MplayUpdatePos()
{
    if (wDeviceID != 0) {
        MCI_STATUS_PARMS mciStatusParms;
        DWORD err;

        mciStatusParms.dwItem = MCI_STATUS_POSITION;
        err = mciSendCommand(wDeviceID,
            MCI_STATUS,
            MCI_WAIT|MCI_STATUS_ITEM,
            (DWORD_PTR)&mciStatusParms);
        if (err != 0) {
            return MCI_ERROR_TO_MPLAY_ERROR(err);
        }
        return MplaySetTrackerPos((DWORD)mciStatusParms.dwReturn);

    }
    return MPLAY_ERROR_NO_MEDIA;
}


int
MplayCloseStream(HWND hWnd)
{
    if (wDeviceID) {
        DWORD err;
        //
        // This may fail if we're not playing anything.
        //
        mciSendCommand(wDeviceID, MCI_STOP, 0, 0); 

        err = mciSendCommand(wDeviceID, MCI_CLOSE, 0, 0);
        if (err != 0) {
            ASSERT(!"MCI_CLOSE failed, check err value");
            return MCI_ERROR_TO_MPLAY_ERROR(err);
        }

        wDeviceID = 0;
        if (MplayIsTracker())
            KillTimer(hWnd,TIMER_ID);
        return MPLAY_ERROR_SUCCESS;
    } else {
        return MPLAY_ERROR_NO_MEDIA;
    }
}

static int
OpenStream(HWND hWnd,LPTSTR szFile)
{
    TCHAR szExt[sizeof(lpLinks[0].szExt) / sizeof(lpLinks[0].szExt[0])];
    LPTSTR a;
    DWORD err;
    MCI_OPEN_PARMS mciOpenParms;

    if (wDeviceID) {
        err = MplayCloseStream(hWnd);
        ASSERT( err == MPLAY_ERROR_SUCCESS );
    }
    memset(&mciOpenParms, 0, sizeof(mciOpenParms));

    a = _tcsrchr(szFile,'.');

    //
    //  Our array of extensions uses fixed sized strings.
    //  If the string is too large for that lookup, then we don't gain
    //  anything attempting to handle it here - lookup will fail
    //  anyway.
    //

    if (a != NULL && _tcslen(a + 1) < sizeof(szExt)/sizeof(szExt[0])) {
        _tcscpy(szExt, a+1);
        _tcsupr(szExt);
        mciOpenParms.lpstrDeviceType = MplayGetHandler(szExt);
    } else {
        mciOpenParms.lpstrDeviceType = NULL;
    }

    mciOpenParms.lpstrElementName = szFile;
    if (mciOpenParms.lpstrDeviceType == NULL) {
        //
        // We have no handler to play this media.  Throw it to MCI and hope MCI
        // can figure it out.  As far as I can tell, this *should* work but
        // essentially never does.  If we're in this path, we're probably about
        // to die.
        //
        err = mciSendCommand(MCI_DEVTYPE_OTHER,
            MCI_OPEN,
            MCI_OPEN_ELEMENT,
            (DWORD_PTR) &mciOpenParms);

        if (err != 0) {

            //
            // To minimize the risk of really dying, try again with a popular
            // handler.  It seems like all DirectX audio goes via MPEGVideo, so
            // if people install things up there, they can now work down here.
            //
            mciOpenParms.lpstrDeviceType = _T("MPEGVideo");

            err = mciSendCommand(MCI_DEVTYPE_OTHER,
                MCI_OPEN,
                MCI_OPEN_TYPE | MCI_OPEN_ELEMENT,
                (DWORD_PTR)&mciOpenParms);
        
            if (err != 0) {

                //
                // If we don't know and can't guess, then we fail.
                //

                MessageBox(hWnd,
                    _T("Could not open output.  Check that an MCI handler is installed for this format.  Use the AddHandler command in your default.msc file to add support."),
                    _T(MPLAY_NAME),
                    MB_ICONEXCLAMATION );
                return MCI_ERROR_TO_MPLAY_ERROR(err);
            }
        }
    } else {
        err = mciSendCommand(MCI_DEVTYPE_OTHER,
            MCI_OPEN,
            MCI_OPEN_TYPE | MCI_OPEN_ELEMENT,
            (DWORD_PTR)&mciOpenParms);
        
        if (err != 0) {
            TCHAR szTemp[200];
            //
            // Failed to open device. Don't close it; just return error.
            // The error display is here for historical reasons and
            // really should move.
            //
            mciGetErrorString(err, szTemp, sizeof(szTemp)/sizeof(szTemp[0]));
            MessageBox( hWnd,
                szTemp,
                _T(MPLAY_NAME),
                MB_ICONSTOP );
            return MCI_ERROR_TO_MPLAY_ERROR(err);
        }
    }
    MplaySetOpenFile(szFile);
    wDeviceID = mciOpenParms.wDeviceID;
    return MPLAY_ERROR_SUCCESS;
}

int MplayPlayMediaFile(HWND hWndNotify, LIST * List,LONG dwNewIndex)
{
    MCI_PLAY_PARMS mciPlayParms;
    MCI_SET_PARMS mciSetParms;
    MCI_STATUS_PARMS mciStatusParms;
    DWORD err;
    int interr;

    if (dwNewIndex >= 0) {
        dwIndex = dwNewIndex;
    } else {
        switch (dwNewIndex) {
        case PLAY_NEXT:
            //
            //  Since this comes from the currently playing track, this
            //  should be safe up to 2g entries in the list.
            //
            dwNewIndex = ++dwIndex;
            if (dwNewIndex == (LONG)GetListLength(List))
                return MPLAY_ERROR_TRACK_OUT_OF_RANGE;
            ASSERT( dwNewIndex < (LONG)GetListLength(List) );
            if (dwNewIndex > (LONG)GetListLength(List))
                return MPLAY_ERROR_TRACK_OUT_OF_RANGE;
            MplayListUiSetActive( GethWndCli(), dwNewIndex);
            break;
        case PLAY_PREV:
            if (dwIndex == 0) return MPLAY_ERROR_TRACK_OUT_OF_RANGE;
            dwNewIndex = --dwIndex;
            MplayListUiSetActive( GethWndCli(), dwNewIndex );
            break;
        case PLAY_SAME:
            dwNewIndex = dwIndex;
            break;
        default:
            //
            // There are exactly three valid negative cases.
            // If this doesn't happen then dwIndex must be
            // defined.
            //
            ASSERT(dwNewIndex >= 0);
            break;
        }
    }
    if (dwNewIndex >= (LONG)GetListLength(List)) {
        return MPLAY_ERROR_TRACK_OUT_OF_RANGE;
    }

    interr = OpenStream(hWndNotify,
        GetListItemU(List, (DWORD)dwNewIndex));
    if (interr != 0) {
        return interr;
    }

    mciSetParms.dwTimeFormat = MCI_FORMAT_MILLISECONDS;
    err = mciSendCommand(wDeviceID,
        MCI_SET,
        MCI_WAIT|MCI_SET_TIME_FORMAT,
        (DWORD_PTR)&mciSetParms);
    if (err != 0) {
        return MCI_ERROR_TO_MPLAY_ERROR(err);
    }

    mciStatusParms.dwItem = MCI_STATUS_LENGTH;
    err = mciSendCommand(wDeviceID,
        MCI_STATUS,
        MCI_WAIT|MCI_STATUS_ITEM,
        (DWORD_PTR)&mciStatusParms);
    if (err != 0) {
        return MCI_ERROR_TO_MPLAY_ERROR(err);
    }

    mciPlayParms.dwCallback = (DWORD_PTR) hWndNotify;
    err = mciSendCommand(wDeviceID,
        MCI_PLAY,
        MCI_NOTIFY,
        (DWORD_PTR)&mciPlayParms);

    if (err != 0) {
        return MCI_ERROR_TO_MPLAY_ERROR(err);
    }

    MplaySetTrackerLength((DWORD)mciStatusParms.dwReturn);
    MplaySetTrackerPos(0);

    if (MplayIsTracker() && IsWindowVisible(GethWndMain()))
        SetTimer(hWndNotify, TIMER_ID, TIMER_TICK, NULL);

    return MPLAY_ERROR_SUCCESS;

}

BOOL MplayIsPlaying()
{
    if (wDeviceID != 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

static int
MplayInitializeVolume(HMIXER * MixHandle,
    PMIXERCONTROLDETAILS MixCD,
    PMIXERCONTROLDETAILS_UNSIGNED MixValue)
{
    DWORD err;
    MIXERLINECONTROLS mlc;
    MIXERCONTROL mc;
    ASSERT(MixHandle != NULL);
    ASSERT(MixCD != NULL);
    ASSERT(MixValue != NULL);

    err = mixerOpen( MixHandle,
        0,  // Bad assumption - need to get this right.
        0,
        0,
        MIXER_OBJECTF_MIXER );
    if (err != MMSYSERR_NOERROR) {
        ASSERT( err == MMSYSERR_NOERROR );
        return MCI_ERROR_TO_MPLAY_ERROR(err);
    }

    memset(&mlc, 0, sizeof(mlc));
    memset(&mc, 0, sizeof(mc));

    mc.cbStruct = sizeof(mc);

    mlc.cbStruct = sizeof(mlc);
    mlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
    mlc.cControls = 1;
    //
    //  FIXME - Is the below assumption correct?  It looks like per-device
    //  lines are numbered 0x00nn0000, and output lines 0xffff00nn.
    //  
    mlc.dwLineID = 0xffff0000;
    mlc.cbmxctrl = sizeof(mc);
    mlc.pamxctrl = &mc;
    
    err = mixerGetLineControls( (HMIXEROBJ)(*MixHandle),
        &mlc,
        MIXER_GETLINECONTROLSF_ONEBYTYPE );
    if (err != MMSYSERR_NOERROR) {
        ASSERT( err == MMSYSERR_NOERROR );
        mixerClose( *MixHandle );
        return MCI_ERROR_TO_MPLAY_ERROR(err);
    }

    ASSERT( mc.dwControlType == MIXERCONTROL_CONTROLTYPE_VOLUME );
    ASSERT( mc.Bounds.dwMinimum == 0 );
    ASSERT( mc.Bounds.dwMaximum == 0xffff );

    MixCD->cbStruct = sizeof(*MixCD);
    MixCD->dwControlID = mc.dwControlID;
    MixCD->cChannels = 1;
    MixCD->cMultipleItems = 0;
    MixCD->paDetails = MixValue;
    MixCD->cbDetails = sizeof(*MixValue);

    MixValue->dwValue = 0;

    return MPLAY_ERROR_SUCCESS;
}

int MplayGetVolume(DWORD *dwVol)
{
    HMIXER hMix;
    MIXERCONTROLDETAILS MixCD;
    MIXERCONTROLDETAILS_UNSIGNED MixValue;
    DWORD err;

    err = MplayInitializeVolume(&hMix, &MixCD, &MixValue);
    if (err != MPLAY_ERROR_SUCCESS) {
        ASSERT(err == MPLAY_ERROR_SUCCESS);
        return err;
    }

    err = mixerGetControlDetails((HMIXEROBJ)hMix,
        &MixCD,
        0 );
    if (err != MMSYSERR_NOERROR) {
        ASSERT(err == MMSYSERR_NOERROR);
        mixerClose(hMix);
        return MCI_ERROR_TO_MPLAY_ERROR(err);
    }

    *dwVol = MixValue.dwValue;

    mixerClose(hMix);
    return MPLAY_ERROR_SUCCESS;
}

int MplaySetVolume(DWORD dwVol)
{
    HMIXER hMix;
    MIXERCONTROLDETAILS MixCD;
    MIXERCONTROLDETAILS_UNSIGNED MixValue;
    DWORD err;

    err = MplayInitializeVolume(&hMix, &MixCD, &MixValue);
    if (err != MPLAY_ERROR_SUCCESS) {
        ASSERT(err == MPLAY_ERROR_SUCCESS);
        return err;
    }

    MixValue.dwValue = dwVol;

    err = mixerSetControlDetails((HMIXEROBJ)hMix,
        &MixCD,
        0 );
    if (err != MMSYSERR_NOERROR) {
        ASSERT(err == MMSYSERR_NOERROR);
        mixerClose(hMix);
        return MCI_ERROR_TO_MPLAY_ERROR(err);
    }

    mixerClose(hMix);
    return MPLAY_ERROR_SUCCESS;
}

