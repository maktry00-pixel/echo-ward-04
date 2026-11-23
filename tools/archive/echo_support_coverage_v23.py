"""Footprint tests against the actual exposed triangle union, not its AABB."""
import numpy as np

def covers(plane, bounds):
    if bounds[5]>plane.get('ceiling_z',float('inf'))-.018:return False
    triangles=plane.get('triangles')
    if triangles is None:return True
    tri=np.asarray(triangles,dtype=float)
    points=np.array([[x,y] for x in np.linspace(bounds[0],bounds[3],5) for y in np.linspace(bounds[1],bounds[4],5)])
    a=tri[:,0];v=tri[:,1]-a;w=tri[:,2]-a
    det=v[:,0]*w[:,1]-v[:,1]*w[:,0];valid=np.abs(det)>1e-12
    a=a[valid];v=v[valid];w=w[valid];det=det[valid]
    if not len(a):return False
    delta=points[:,None,:]-a
    u=(delta[:,:,0]*w[None,:,1]-delta[:,:,1]*w[None,:,0])/det
    t=(v[None,:,0]*delta[:,:,1]-v[None,:,1]*delta[:,:,0])/det
    return bool(np.all(np.any((u>=-1e-6)&(t>=-1e-6)&(u+t<=1.000001),axis=1)))

def restore(plane, native):
    if 'source_plane' not in plane:return plane
    source=native[str(plane['source_mesh'])][plane['source_plane']]
    xy=np.asarray(source['triangles']);vertices=np.concatenate([xy,np.full((*xy.shape[:2],1),source['z'])],axis=2)
    matrix=np.asarray(plane['to_world'])
    result=dict(plane);result['triangles']=(vertices@matrix[:,:3].T+matrix[:,3])[:,:,:2]
    higher=[p['z'] for p in native[str(plane['source_mesh'])] if p['z']>source['z']+.035]
    if higher:result['ceiling_z']=float(matrix[2,2]*min(higher)+matrix[2,3])
    return result
