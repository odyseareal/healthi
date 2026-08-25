-- Run this entire file in Supabase > SQL Editor.
-- The table is not directly accessible. Passwordless RPCs require one exact,
-- validated device MAC at a time and do not provide a list-devices operation.

create table if not exists public.healthi_devices (
  device_id text primary key
    check (device_id ~ '^[0-9A-F]{2}(:[0-9A-F]{2}){5}$'),
  metrics jsonb not null default '{}'::jsonb,
  website_data jsonb not null default '{"days":[],"foods":[],"moods":[],"workouts":[]}'::jsonb,
  updated_at timestamptz not null default now()
);

alter table public.healthi_devices enable row level security;
revoke all on table public.healthi_devices from anon, authenticated;

create or replace function public.get_healthi_device(p_device_id text)
returns table(metrics jsonb, website_data jsonb, updated_at timestamptz)
language sql
security definer
set search_path = ''
as $$
  select d.metrics, d.website_data, d.updated_at
  from public.healthi_devices d
  where p_device_id ~ '^[0-9A-F]{2}(:[0-9A-F]{2}){5}$'
    and d.device_id = p_device_id
  limit 1;
$$;

create or replace function public.upsert_healthi_metrics(p_device_id text, p_metrics jsonb)
returns void
language plpgsql
security definer
set search_path = ''
as $$
begin
  if p_device_id !~ '^[0-9A-F]{2}(:[0-9A-F]{2}){5}$' then
    raise exception 'Invalid Healthi device ID';
  end if;
  insert into public.healthi_devices(device_id, metrics, updated_at)
  values (p_device_id, coalesce(p_metrics, '{}'::jsonb), now())
  on conflict (device_id) do update
    set metrics = excluded.metrics, updated_at = now();
end;
$$;

create or replace function public.upsert_healthi_website_data(p_device_id text, p_website_data jsonb)
returns void
language plpgsql
security definer
set search_path = ''
as $$
begin
  if p_device_id !~ '^[0-9A-F]{2}(:[0-9A-F]{2}){5}$' then
    raise exception 'Invalid Healthi device ID';
  end if;
  insert into public.healthi_devices(device_id, website_data, updated_at)
  values (p_device_id, coalesce(p_website_data, '{}'::jsonb), now())
  on conflict (device_id) do update
    set website_data = excluded.website_data, updated_at = now();
end;
$$;

revoke all on function public.get_healthi_device(text) from public;
revoke all on function public.upsert_healthi_metrics(text, jsonb) from public;
revoke all on function public.upsert_healthi_website_data(text, jsonb) from public;
grant execute on function public.get_healthi_device(text) to anon, authenticated;
grant execute on function public.upsert_healthi_metrics(text, jsonb) to anon, authenticated;
grant execute on function public.upsert_healthi_website_data(text, jsonb) to anon, authenticated;
