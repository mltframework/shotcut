function setFontSize(size) {
    var randomCssClass = "rangyTemp_" + (+new Date());
    var classApplier = rangy.createCssClassApplier(randomCssClass, true);
    classApplier.applyToSelection();

    // Now use jQuery to add the CSS colour and remove the class
    $("." + randomCssClass).css({"font-size": size}).removeClass(randomCssClass);
}

function formatTextOutline(stroke) {
    var randomCssClass = "rangyTemp_" + (+new Date());
    var classApplier = rangy.createCssClassApplier(randomCssClass, true);
    classApplier.applyToSelection();

    // Now use jQuery to add the CSS colour and remove the class
    $("." + randomCssClass).css({"-webkit-text-stroke": stroke}).removeClass(randomCssClass);
}

function formatTextShadow(shadow) {
    var randomCssClass = "rangyTemp_" + (+new Date());
    var classApplier = rangy.createCssClassApplier(randomCssClass, true);
    classApplier.applyToSelection();

    // Now use jQuery to add the CSS colour and remove the class
    $("." + randomCssClass).css({"text-shadow": shadow}).removeClass(randomCssClass);
}

document.onload = function() {
    rangy.init();
};
