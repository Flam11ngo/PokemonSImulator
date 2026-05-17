import json,re
f=open('data/items.json');d=json.load(f)
with open('include/battle/Items.h') as hf:
    htext = hf.read()
m=re.search(r'enum class ItemType\s*\{(.*?)\};', htext, re.DOTALL)
engine_names=set()
if m:
    for line in m.group(1).split('\n'):
        line=line.strip().strip(',')
        if line and not line.startswith('//') and line!='Count':
            engine_names.add(line.split('=')[0].strip().lower())
battle=[i for i in d['items'] if 'held' in i.get('description','').lower() or 'holder' in i.get('description','').lower()]
missing=[i for i in battle if i['name'].replace(' ','').replace('-','').replace("'",'').lower() not in engine_names]
print('Total items:',len(d['items']))
print('Battle items:',len(battle))
print('Missing:',len(missing))
