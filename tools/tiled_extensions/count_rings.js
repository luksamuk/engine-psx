/// <reference types="@mapeditor/tiled-api" />

// Put this in .config/tiled/extensions

let countRings = tiled.registerAction("countRings", function(action) {
    const asset = tiled.activeAsset;
    if (!asset || !asset.isTileMap) {
        tiled.alert("No active map.");
        return;
    }

    let ringCount = 0;
    let monitorRings = 0;

    asset.layers.forEach(grp => {
        if(grp.isGroupLayer && grp.name.toUpperCase() === "OBJECTS") {
            grp.layers.forEach(layer => {
                if (!layer.isObjectLayer && layer.name.toUpperCase() !== "COMMON" ) return;
                layer.objects.forEach(obj => {
                    const type = obj.tile.className.toLowerCase() || "";
                    if (type === "ring") {
                        ringCount += 1;
                    } else if (type === "ring_3h" || type === "ring_3v") {
                        ringCount += 3;
                    } else if (type === "monitor") {
                        const kind = obj.property("Kind") || "";
                        if (kind.toUpperCase() === "RING") {
                            monitorRings += 10;
                        }
                    }
                });
            });
        }
    });

    const total = ringCount + monitorRings;

    tiled.alert(
        "Rings (objects): " + ringCount +
            "\nRings (monitors): " + monitorRings +
            "\nTotal Rings: " + total
    );
});
countRings.text = "Count Rings";
countRings.shortcut = "Ctrl+Alt+R";

tiled.extendMenu("Map", [
    { action: "countRings", before: "MapProperties" },
    { separator: true }
]);
