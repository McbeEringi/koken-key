#!/bin/env node
const
	arg=process.argv.slice(2),
	te=new TextEncoder(),
	le4=x=>[x&0xff,(x>>>8)&0xff,(x>>>16)&0xff,(x>>>24)&0xff],
	crc=(t=>(buf,crc=0)=>~buf.reduce((c,x)=>t[(c^x)&0xff]^(c>>>8),~crc))([...Array(256)].map((_,n)=>[...Array(8)].reduce(c=>((c&1)?0xedb88320:0)^(c>>>1),n))),// https://www.rfc-editor.org/rfc/rfc1952
	hash=async()=>await(await fetch('http://koken-key.local/hash')).text(),
	key=async x=>(crc([...te.encode(arg[0]),...le4(+('0x'+(x||await hash())))])>>>0).toString(16).padStart(8,0),
	act=async act=>await fetch('http://koken-key.local',{method:'POST',headers:{key:await key(),act}});
await act(arg[1]);
