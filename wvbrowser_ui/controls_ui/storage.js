function handleUpgradeEvent(event) {
    console.log('Creating DB');
    let newDB = event.target.result;

    newDB.onerror = function(event) {
        console.log('Something went wrong');
        console.log(event);
    };

    let newFavoritesStore = newDB.createObjectStore('favorites', {
        keyPath: 'uri'
    });

    newFavoritesStore.transaction.oncomplete = function(event) {
        console.log('Object stores created');
    };

    let newHistoryStore = newDB.createObjectStore('history', {
        autoIncrement: true
    });

    newHistoryStore.createIndex('timestamp', 'timestamp', {
        unique: false
    });

    newHistoryStore.createIndex('stampedURI', ['uri', 'timestamp'], {
        unique: false
    });
}

// The database stays open for the queries that follow. Each query used to
// open it, several times per page loaded, and none closed it: each left a
// connection open in the storage service until garbage collection.
let openDB = null;
let queriesWaitingForDB = null;

function queryDB(query) {
    if (openDB) {
        query(openDB);
        return;
    }

    // Opening already: the query runs once it's open
    if (queriesWaitingForDB) {
        queriesWaitingForDB.push(query);
        return;
    }
    queriesWaitingForDB = [query];

    let request = window.indexedDB.open('WVBrowser');

    request.onerror = function(event) {
        console.log('Failed to open database');
        queriesWaitingForDB = null;
    };

    request.onsuccess = function(event) {
        let db = event.target.result;

        // Opened again by the next query if the browser closes it
        db.onclose = function(event) {
            openDB = null;
        };

        openDB = db;
        let queries = queriesWaitingForDB;
        queriesWaitingForDB = null;
        queries.map(waitingQuery => waitingQuery(db));
    };

    request.onupgradeneeded = handleUpgradeEvent;
}
