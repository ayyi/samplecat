$(function(){
	setTimeout(function(){
		//console.log("doing table..." + $.fn.dataTable);
	}, 2000);

	$('.btn').click(function(){
		location.href = "/search";
	});
});

function do_table(){
	$('table#samples').dataTable({
		bPaginate: false,
		bFilter: true,
	});
}
