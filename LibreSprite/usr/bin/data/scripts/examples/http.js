const methods = {
    init(){
        storage.fetch("https://raw.githubusercontent.com/LibreSprite/LibreSprite/master/README.md", "read_me");
    },
    read_me_fetch(){
        console.log("Read me: " + storage.get("read_me"));
    }
};

function onEvent(event) {
    if (event in methods)
        methods[event]();
}
