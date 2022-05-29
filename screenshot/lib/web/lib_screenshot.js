// https://kripken.github.io/emscripten-site/docs/porting/connecting_cpp_and_javascript/Interacting-with-code.html

var LibScreenshot = {
	screenshot_on_the_next_frame: function(callback, x, y, w, h) {
		var requestAnimationFrameSub = window.requestAnimationFrame;
		window.requestAnimationFrame = function(f) {
			w = w || canvas.width;
			h = h || canvas.height;

			var hidden_canvas = document.createElement("canvas");
			hidden_canvas.style.display = "none";
			document.body.appendChild(hidden_canvas);
			hidden_canvas.width = w;
			hidden_canvas.height = h;

			var context = hidden_canvas.getContext("2d");
			var sx = x;
			var sy = canvas.height - (y + h);
			var sw = w;
			var sh = h;

			var dx = 0;
			var dy = 0;
			var dw = w;
			var dh = h;
			context.drawImage(canvas, sx, sy, sw, sh, dx, dy, dw, dh);

			// img_rgba is `Uint8ClampedArray` representing a one-dimensional array containing
			// the data in the RGBA order, with integer values between 0 and 255 (inclusive).
			var img_rgba = context.getImageData(dx, dy, dw, dh);
			var img_buf = Module._malloc(img_rgba.data.length * img_rgba.data.BYTES_PER_ELEMENT);
			Module.HEAPU8.set(img_rgba.data, img_buf);
			document.body.removeChild(hidden_canvas);

			setTimeout(function() {
				{{{ makeDynCall("viiii", "callback") }}} (img_buf, w, h);
				Module._free(img_buf);
			}, 0);

			window.requestAnimationFrame = requestAnimationFrameSub;
			requestAnimationFrameSub(f);
		}
	}
}

mergeInto(LibraryManager.library, LibScreenshot);
