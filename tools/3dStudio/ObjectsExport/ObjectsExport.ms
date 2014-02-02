filename = getsavefilename caption:"Objects Export"

if filename != undefined then (
	
	file = createfile filename
	
	for node in selection do (
		format "%\n" node.name to:file
		
		xyz = node.transform.position
		rot = (inverse node.transform).rotation
		format "% % % % % % %\n\n" xyz.x xyz.y xyz.z rot.x rot.y rot.z rot.w to:file
	)
	
	close file
)
