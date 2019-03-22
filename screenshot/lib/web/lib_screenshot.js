// https://kripken.github.io/emscripten-site/docs/porting/connecting_cpp_and_javascript/Interacting-with-code.html

var LibScreenshot = {
	screenshot_on_the_next_frame: function(callback) {
		var requestAnimationFrameSub = window.requestAnimationFrame;
		window.requestAnimationFrame = function(f) {
		   	var image = canvas.toDataURL("image/png");
	    	requestAnimationFrameSub(f);
		    window.requestAnimationFrame = requestAnimationFrameSub
		    var bs64_img = allocate(intArrayFromString(image), "i8", ALLOC_NORMAL);
		    setTimeout(function() {
			    Runtime.dynCall("vi", callback, [bs64_img]);
			    Module._free(bs64_img);
		    }, 0);
		}
	}
}

mergeInto(LibraryManager.library, LibScreenshot);