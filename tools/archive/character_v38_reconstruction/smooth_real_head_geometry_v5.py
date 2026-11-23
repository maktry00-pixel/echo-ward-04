from pathlib import Path
p=Path(__file__).parent/'fit_real_head_geometry_v5.py'
s=p.read_text(encoding='utf8')
s=s.replace("dy=np.clip(desired-original,-.012,.012)","raw=desired-original\n grid=np.arange(-40,41);gauss=np.exp(-.5*(grid/13.)**2);gauss/=gauss.sum()\n dy=np.convolve(np.pad(raw,(40,40),mode='edge'),gauss,mode='valid')\n dy=np.clip(dy,-.009,.009)")
s=s.replace("Beka_head_geometry_study_v5","Beka_head_geometry_study_v5b").replace("'fitted_front'","'smoothed_front'").replace("'fitted_nose_right'","'smoothed_nose_right'").replace("'fitted_nose_left'","'smoothed_nose_left'").replace("'fitted_three_quarter'","'smoothed_three_quarter'")
s=s.replace("'fit_report.json'","'fit_report_smoothed.json'")
exec(compile(s,str(p),'exec'))

