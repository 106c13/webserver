(function () {
    const status = document.getElementById("js-status");
    const consoleBox = document.getElementById("console");

    function log(msg) {
        consoleBox.textContent += msg + "\n";
    }

    log("[BOOT] Initializing JS module...");
    log("[OK] JavaScript loaded successfully");

    status.innerHTML =
        "▸ STATUS: <span style='color:#22d3ee'>JS RUNNING</span>";

    let tick = 0;

    setInterval(() => {
        tick++;
        log("[PING] JS alive for " + tick + "s");
        consoleBox.scrollTop = consoleBox.scrollHeight;
    }, 1000);
})();

