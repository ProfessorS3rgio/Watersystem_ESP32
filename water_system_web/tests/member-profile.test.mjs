import test from 'node:test'
import assert from 'node:assert/strict'
import { memberDraft, memberPayload, fullMemberName, memberAge } from '../resources/js/utils/memberProfile.js'
import { useMemberProfile } from '../resources/js/composables/useMemberProfile.js'

const record = (id = 1, profile = null) => ({ customer:{id, full_name:`Existing member ${id}`, address:'Makilas'}, profile, revision:`revision-${id}` })
test('legacy names remain whole and blank personal details stay blank', () => {
  const draft = memberDraft(record())
  assert.equal(draft.full_name,'Existing member 1')
  assert.equal(draft.first_name,'')
  assert.equal(draft.last_name,'')
  assert.equal(memberPayload(draft).full_name,'Existing member 1')
  assert.equal(memberPayload(draft).birthdate,null)
})
test('name parts compose a single name without damaging Unicode or compound surnames', () => {
  const draft = {...memberDraft(record()),first_name:' María ',middle_name:'de la Paz',last_name:"D'Angelo",name_suffix:'Jr.'}
  assert.equal(fullMemberName(draft),"María de la Paz D'Angelo Jr.")
  assert.equal(memberPayload(draft).full_name,"María de la Paz D'Angelo Jr.")
  assert.equal(memberAge('1990-10-27','2026-10-26'),35)
  assert.equal(memberAge('1990-10-26','2026-10-26'),36)
  assert.equal(memberAge(null,'2026-10-26'),null)
})
test('a failed save preserves the draft and a retry adopts the server revision', async () => {
  let fail = true
  const flow = useMemberProfile({get:async()=>record(),save:async(id,payload)=>{
    assert.equal(payload.revision,'revision-1')
    if(fail) throw Object.assign(new Error('Meter number already assigned'),{errors:{meter_number:['Choose another meter number']}})
    return {...record(id,{meter_number:payload.meter_number}),revision:'saved-revision'}
  }})
  await flow.load(1)
  flow.draft.value.meter_number='WM-100'
  await flow.save()
  assert.equal(flow.draft.value.meter_number,'WM-100')
  assert.equal(flow.dirty.value,true)
  assert.equal(flow.errors.value.meter_number[0],'Choose another meter number')
  assert.equal(flow.saving.value,false)
  fail=false
  await flow.save()
  assert.equal(flow.record.value.revision,'saved-revision')
  assert.equal(flow.dirty.value,false)
  assert.equal(flow.saved.value,true)
})
test('late responses cannot replace a different member’s profile', async () => {
  let finishFirst
  const flow = useMemberProfile({get:id=>id===1?new Promise(resolve=>{finishFirst=resolve}):Promise.resolve(record(2))})
  const first = flow.load(1)
  await flow.load(2)
  finishFirst(record(1))
  await first
  assert.equal(flow.record.value.customer.id,2)
  assert.equal(flow.loading.value,false)
})
test('duplicate saves are blocked and discard restores the saved details', async () => {
  let finish, writes=0
  const flow = useMemberProfile({get:async()=>record(),save:async()=>{writes++;return new Promise(resolve=>{finish=resolve})}})
  await flow.load(1)
  flow.draft.value.phone='09170000000'
  const saving = flow.save()
  await flow.save()
  assert.equal(writes,1)
  finish(record(1,{phone:'09170000000'}))
  await saving
  flow.draft.value.phone='09170000001'
  flow.discard()
  assert.equal(flow.draft.value.phone,'09170000000')
  assert.equal(flow.dirty.value,false)
})
