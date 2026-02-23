(function () {
    const terminal = document.getElementById("terminal");
    const bigMsg = document.getElementById("big-message");

    const commands = [
        "boot sequence start...",
        "loading kernel modules...",
        "mounting /dev/webserv",
        "checking filesystem... OK",
        "whoami",
        "root",
        "ls /var/www",
        "index.html  about.html  js-terminal.html",
        "cat server.info",
        "WebServ 42 — custom HTTP server",
        "connecting to localhost:8080...",
        "handshake complete"
    ];

    let i = 0;

    function printLine(text) {
        const line = document.createElement("div");
        line.className = "line";
        line.textContent = "> " + text;
        terminal.appendChild(line);
        terminal.scrollTop = terminal.scrollHeight;
    }

    function nextCommand() {
        if (i < commands.length) {
            printLine(commands[i]);
            i++;
            setTimeout(nextCommand, 400);
        } else {
            setTimeout(showBigMessage, 800);
        }
    }

    function showBigMessage() {
        bigMsg.classList.remove("hidden");
    }

    setTimeout(nextCommand, 600);
})();

