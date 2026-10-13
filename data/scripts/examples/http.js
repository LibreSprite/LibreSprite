
///permission fetch
// Legacy storage.fetch

const methods = {
    init(){
        console.log('storage.fetch');
        storage.fetch("https://raw.githubusercontent.com/LibreSprite/LibreSprite/master/README.md", "read_me");
    },
    read_me_fetch(){
        // Status 0 means the request failed; log the value length, not
        // the full body (keeps the console output on one line).
        console.log("Storage fetch: status=" + storage.get("read_me_status") +
                    " len=" + (storage.get("read_me") || "").length);
    }
};

function onEvent(event) {
    console.log('Event:', event);
    if (event in methods)
        methods[event]();
}


// Standard fetch

async function main() {
    const rsp = await fetch('https://raw.githubusercontent.com/LibreSprite/LibreSprite/master/README.md');
    const txt = await rsp.text();
    console.log('Web Fetch: len=' + txt.length + ' ' + txt.slice(0, 40));
}

main().catch(ex => {
    console.log('EX:', ex.message, ex.stack);
});

const server = new HTTPServer({
    port: 9999,
    onGet({path, method, body, headers, query}) {
        console.log(JSON.stringify({path, method, body, headers, query}, null, 2));
        return "GetBacon";
    },
    onPost({path, method, body, headers, query}) {
        console.log(JSON.stringify({path, method, body, headers, query}, null, 2));
        return "PostBacon";
    }
});
server.start();
