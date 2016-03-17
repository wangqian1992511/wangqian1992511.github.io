document.write("<style type='text/css'>.hidden{display:none}<\/style>") //hide all texts, except when javascript is off.
function show(area){
    var obj = document.getElementById(area)
    obj.style.display=(obj.style.display=='block')?'none':'block'
}

function goTopEx() { 
	var obj = document.getElementById("goTopBtn"); 
	function getScrollTop() { 
		return document.documentElement.scrollTop + document.body.scrollTop; 
	} 
	function setScrollTop(value) { 
		if (document.documentElement.scrollTop) { 
			document.documentElement.scrollTop = value; 
		} else { 
			document.body.scrollTop = value; 
		} 
	} 
	window.onscroll = function() { 
		getScrollTop() > 0 ? obj.style.display = "": obj.style.display = "none"; 
	} 
	obj.onclick = function() { 
		var goTop = setInterval(scrollMove, 10); 
		function scrollMove() { 
			setScrollTop(getScrollTop() / 1.1); 
			if (getScrollTop() < 1) clearInterval(goTop); 
		} 
	} 
} 