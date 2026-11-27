(() => {
  'use strict';
  const $ = (s, root=document) => root.querySelector(s);
  const $$ = (s, root=document) => [...root.querySelectorAll(s)];
  const defaults = {
    plugins: [
      {id:1,name:'Homebrew Toolbox',file:'homebrew-toolbox.prx',notes:'Personal reference entry for homebrew utilities.'},
      {id:2,name:'Overlay Notes',file:'overlay-notes.prx',notes:'Keep a note of your preferred overlay setup.'},
      {id:3,name:'System Monitor',file:'system-monitor.prx',notes:'Reference entry for system monitoring tools.'}
    ],
    settings:{compact:false,motion:true,notifications:true,remember:true}
  };
  const load = () => {
    try { const raw=localStorage.getItem('vypex-workspace-v020'); return raw?{...defaults,...JSON.parse(raw)}:structuredClone(defaults); }
    catch { return structuredClone(defaults); }
  };
  let state=load(), toastTimer;
  const save=()=>{try{if(state.settings.remember)localStorage.setItem('vypex-workspace-v020',JSON.stringify(state));else localStorage.removeItem('vypex-workspace-v020')}catch{}};
  const notify=(msg)=>{const el=$('#toast');el.textContent=msg;el.classList.add('show');clearTimeout(toastTimer);toastTimer=setTimeout(()=>el.classList.remove('show'),2300)};
  const go=(view)=>{ $$('.page').forEach(p=>p.classList.toggle('active',p.id==='view-'+view));$$('.nav-item').forEach(b=>b.classList.toggle('active',b.dataset.view===view));$('#crumb-current').textContent=view.toUpperCase();if(location.hash!=='#'+view)history.replaceState(null,'','#'+view); };
  const renderPlugins=(filter='')=>{
    const list=$('#plugin-list'),items=state.plugins.filter(p=>(p.name+' '+p.file+' '+p.notes).toLowerCase().includes(filter.toLowerCase()));
    list.innerHTML=items.length?items.map(p=>`<article class="plugin-card"><div class="plugin-card-top"><span class="plugin-symbol">▦</span><span class="entry-state">● SAVED ENTRY</span></div><h3>${escapeHtml(p.name)}</h3><p>${escapeHtml(p.notes||'No notes added.')}</p><span class="plugin-file">${escapeHtml(p.file||'No file reference')}</span><div class="plugin-actions"><button class="text-btn" data-edit="${p.id}">Edit</button><button class="text-btn danger" data-delete="${p.id}">Remove</button></div></article>`).join(''):'<div class="panel"><p>No saved entries match that search.</p></div>';
    $('#plugin-count').textContent=String(state.plugins.length).padStart(2,'0');
  };
  const escapeHtml=s=>String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const applySettings=()=>{document.body.classList.toggle('compact',!!state.settings.compact);document.body.classList.toggle('no-motion',!state.settings.motion);$$('[data-setting]').forEach(el=>el.checked=!!state.settings[el.dataset.setting]);};
  $$('.nav-item').forEach(b=>b.addEventListener('click',()=>go(b.dataset.view)));
  $$('[data-go]').forEach(b=>b.addEventListener('click',()=>go(b.dataset.go)));
  $('#plugin-search').addEventListener('input',e=>renderPlugins(e.target.value));
  $('#add-plugin').addEventListener('click',()=>{ $('#plugin-form').reset();$('#plugin-form').dataset.edit='';$('#plugin-dialog').showModal(); });
  $('#plugin-form').addEventListener('submit',e=>{
    e.preventDefault();const form=e.currentTarget,name=$('#plugin-name').value.trim();if(!name)return;
    const item={name,file:$('#plugin-file').value.trim(),notes:$('#plugin-notes').value.trim()};
    const id=Number(form.dataset.edit);if(id){state.plugins=state.plugins.map(p=>p.id===id?{...p,...item}:p);notify('Plugin entry updated');}else{item.id=Date.now();state.plugins.unshift(item);notify('Plugin entry added');}
    save();renderPlugins($('#plugin-search').value);$('#plugin-dialog').close();
  });
  $('#plugin-list').addEventListener('click',e=>{
    const del=e.target.closest('[data-delete]'),edit=e.target.closest('[data-edit]');
    if(del){state.plugins=state.plugins.filter(p=>p.id!==Number(del.dataset.delete));save();renderPlugins($('#plugin-search').value);notify('Entry removed from this browser');}
    if(edit){const p=state.plugins.find(x=>x.id===Number(edit.dataset.edit));if(!p)return;$('#plugin-name').value=p.name;$('#plugin-file').value=p.file;$('#plugin-notes').value=p.notes;$('#plugin-form').dataset.edit=p.id;$('#plugin-dialog').showModal();}
  });
  $$('[data-setting]').forEach(el=>el.addEventListener('change',()=>{state.settings[el.dataset.setting]=el.checked;save();applySettings();notify('Preference saved');}));
  $('#reset-settings').addEventListener('click',()=>{state.settings={...defaults.settings};save();applySettings();notify('Preferences reset');});
  $('#help-btn').addEventListener('click',()=>notify('Vypex local interface · No PS4 connection active'));
  window.addEventListener('hashchange',()=>{const v=location.hash.slice(1);if(['overview','plugins','settings','about'].includes(v))go(v)});
  applySettings();renderPlugins();const initial=location.hash.slice(1);go(['overview','plugins','settings','about'].includes(initial)?initial:'overview');
})();