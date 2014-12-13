#include <Windows.h>
#include <wuapi.h>

#include "pluginapi.h"

#ifdef __cplusplus
extern "C" {
#endif

extern TCHAR buffer[NSIS_VARSIZE];

void __declspec(dllexport)
nsInstallPrintDriver(HWND hwndParent, int string_size,
        LPTSTR variables, stack_t **stacktop)
{
    HRESULT hr;
	long i, count, retval;

    IUpdateSession* pUpdateSession = NULL;
    IUpdateServiceManager* pUpdateServiceManager = NULL;
    IUpdateService* pUpdateService = NULL;
    IUpdateSearcher* pUpdateSearcher = NULL;
    ISearchResult *pSearchResult = NULL;
	IUpdateCollection *pSearchCollection = NULL;
    IUpdateCollection *pUpdateCollection = NULL;
    IUpdateDownloader *pUpdateDownloader = NULL;
	IDownloadResult *pDownloadResult = NULL;
    IUpdateInstaller *pUpdateInstaller = NULL;
	IInstallationResult *pInstallationResult = NULL;
    LPWSTR title = NULL;

    EXDLL_INIT();
    CoInitialize(NULL);

    /* Pop driver name into buffer. */
    popstring(buffer);

    hr = CoCreateInstance(
            CLSID_UpdateSession,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IUpdateSession,
            (void **)
            &pUpdateSession
        );

    if (FAILED(hr) || pUpdateSession == NULL) {
        lstrcpy(buffer, TEXT("Error creating update session."));
        setuservariable(INST_R0, TEXT("0"));
        pushstring(buffer);
        goto cleanup;
    }

    hr = CoCreateInstance(
            CLSID_UpdateServiceManager,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IUpdateServiceManager,
            (void **) &pUpdateServiceManager
        );

    if (FAILED(hr) || pUpdateServiceManager == NULL) {
        lstrcpy(buffer, TEXT("Error creating update service manager."));
        setuservariable(INST_R0, TEXT("0"));
        pushstring(buffer);
        goto cleanup;
    }

    hr = pUpdateSession->CreateUpdateSearcher(&pUpdateSearcher);

    if (FAILED(hr) || pUpdateSearcher == NULL) {
        lstrcpy(buffer, TEXT("Error creating update searcher."));
        setuservariable(INST_R0, TEXT("0"));
        pushstring(buffer);
        goto cleanup;
    }

    pUpdateSearcher->put_ServerSelection(ssWindowsUpdate);
    pUpdateSearcher->put_CanAutomaticallyUpgradeService(VARIANT_TRUE);
    pUpdateSearcher->put_IncludePotentiallySupersededUpdates(VARIANT_TRUE);
    pUpdateSearcher->put_Online(VARIANT_TRUE);

    hr = pUpdateSearcher->Search(
            BSTR(TEXT("IsInstalled = 0 AND IsHidden = 0")),
            &pSearchResult
        );

    if (FAILED(hr) || pSearchResult == NULL) {
        lstrcpy(buffer, TEXT("Error fetching driver list."));
        setuservariable(INST_R0, TEXT("0"));
        pushstring(buffer);
        goto cleanup;
    }

	hr = CoCreateInstance(
		CLSID_UpdateCollection,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IUpdateCollection,
		(void **)&pSearchCollection
	);

    if (FAILED(hr) || pSearchCollection == NULL) {
        lstrcpy(buffer, TEXT("Error initializing search collection."));
        setuservariable(INST_R0, TEXT("0"));
        pushstring(buffer);
        goto cleanup;
    }

	pSearchResult->get_Updates(&pSearchCollection);
	pSearchCollection->get_Count(&count);
    for (i = 0; i < count - 1; i++) {
		IUpdate *update = NULL;
		hr = CoCreateInstance(
			CLSID_NULL,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IUpdate,
			(void **)&update
		);

		pSearchCollection->get_Item(i, &update);
		update->get_Title(&title);
        if (wcscmp(CharUpper(title), CharUpper(buffer)) == 0)
        {
			hr = CoCreateInstance(
				CLSID_UpdateCollection,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_IUpdateCollection,
				(void **)&pUpdateCollection
			);
            pUpdateCollection->Add(update, &retval);
        }
    }

    hr = pUpdateSession->CreateUpdateDownloader(&pUpdateDownloader);
    hr = pUpdateDownloader->put_Updates(pUpdateCollection);
    hr = pUpdateDownloader->Download(&pDownloadResult);

    hr = pUpdateSession->CreateUpdateInstaller(&pUpdateInstaller);
    pUpdateInstaller->put_Updates(pUpdateCollection);
    pUpdateInstaller->Install(&pInstallationResult);

    setuservariable(INST_R0, TEXT("1"));

cleanup:
    delete pUpdateSession;
    delete pUpdateServiceManager;
    delete pUpdateService;
    delete pUpdateSearcher;
    delete pSearchResult;
    delete pSearchCollection;
    delete pUpdateCollection;
    delete pUpdateDownloader;
    delete pDownloadResult;
    delete pUpdateInstaller;
    delete pInstallationResult;
    delete title;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
