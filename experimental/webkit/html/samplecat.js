$(document).ready(function(){
	setTimeout(function(){
		//console.log("doing table..." + $.fn.dataTable);
		/*
		*/
	}, 2000);
});

function do_table(){
	$('table#samples').dataTable({
		bPaginate: false,
		bFilter: true,
	});
}
