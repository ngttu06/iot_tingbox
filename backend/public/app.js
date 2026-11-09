const $=id=>document.getElementById(id);
let order=null,mock=false;
async function api(path,body){
  const r=await fetch(path,{method:body?'POST':'GET',headers:{Authorization:`Bearer ${$('token').value}`,...(body?{'Content-Type':'application/json'}:{})},...(body?{body:JSON.stringify(body)}:{})});
  const data=await r.json();if(!r.ok)throw Error(data.message||data.error);return data;
}
function show(o){order=o;$('order').hidden=false;$('order-id').textContent=o.id;$('status').textContent=`${o.amount.toLocaleString('vi-VN')} đ · ${o.status}`;$('qr').src=o.qr_image;$('qr').hidden=o.status!=='PENDING';$('simulate').hidden=!mock||o.status!=='PENDING';}
function failure(e){$('message').textContent=e.message;}
$('order-form').addEventListener('submit',async e=>{e.preventDefault();try{show(await api('/v1/orders',{amount:Number($('amount').value),device_id:$('device').value}));$('message').textContent='';}catch(e){failure(e);}});
$('simulate').onclick=async()=>{try{
  const date=new Intl.DateTimeFormat('en-GB',{timeZone:'Asia/Ho_Chi_Minh',day:'2-digit',month:'2-digit',year:'numeric',hour:'2-digit',minute:'2-digit',second:'2-digit',hourCycle:'h23'}).format(new Date()).replace(',','');
  await api('/dev/simulate-credit',{transactionID:crypto.randomUUID(),amount:String(order.amount),description:order.memo,transactionDate:date,type:'IN'});
  show(await api(`/v1/orders/${order.id}`));
}catch(e){failure(e);}};
$('refresh').onclick=async()=>{try{
  $('sync').textContent=JSON.stringify(await api('/v1/admin/sync-status'),null,2);
  const history=await api('/v1/transactions');$('history').replaceChildren();
  for(const t of history){const p=document.createElement('p');p.className='transaction';p.textContent=`${t.transaction_at} · ${t.amount.toLocaleString('vi-VN')} đ · ${t.matching_status}\n${t.content}`;$('history').appendChild(p);}
}catch(e){failure(e);}};
fetch('/healthz').then(r=>r.json()).then(h=>{mock=h.mode==='mock';$('mode').textContent=mock?'MOCK — tài khoản thử nghiệm, không chuyển tiền thật':'Production';}).catch(failure);
setInterval(async()=>{if(order?.status==='PENDING')try{show(await api(`/v1/orders/${order.id}`));}catch(e){failure(e);}},3000);