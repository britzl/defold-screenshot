// https://kripken.github.io/emscripten-site/docs/porting/connecting_cpp_and_javascript/Interacting-with-code.html

var LibScreenshot = {
	screenshot_on_the_next_frame: function(callback, x, y, w, h) {
		var requestAnimationFrameSub = window.requestAnimationFrame;
		window.requestAnimationFrame = function(f) {
			var image;
			if (w > 0 && h > 0) {
				var hidden_canvas = document.createElement('canvas');
				hidden_canvas.style.display = 'none';
				document.body.appendChild(hidden_canvas);
				hidden_canvas.width = w;
				hidden_canvas.height = h;

				var context = hidden_canvas.getContext('2d');
				var sx = x;
				var sy = canvas.height - (y + h);
				var sw = w;
				var sh = h;

				var dx = 0;
				var dy = 0;
				var dw = w;
				var dh = h;
				context.drawImage(canvas, sx, sy, sw, sh, dx, dy, dw, dh);

				image = hidden_canvas.toDataURL("image/png");

				document.body.removeChild(hidden_canvas)
			} else {
				image = canvas.toDataURL("image/png");
			}

			requestAnimationFrameSub(f);
			window.requestAnimationFrame = requestAnimationFrameSub;
			var bs64_img = allocate(intArrayFromString(image), "i8", ALLOC_NORMAL);
			setTimeout(function() {
				{{{ makeDynCall("vii", "callback") }}} (bs64_img);
				Module._free(bs64_img);
			}, 0);
		}
	}
}

mergeInto(LibraryManager.library, LibScreenshot);
