$json = Get-Content 'd:\WeChat-Hook\PCWeixinHook_ShowDoc_All_Apifox.json' -Raw | ConvertFrom-Json
$total = 0
foreach ($cat in $json.item) {
    Write-Host ("`n=== {0} ({1}) ===" -f $cat.name, $cat.item.Count)
    foreach ($req in $cat.item) {
        $path = $req.request.url.path -join '/'
        Write-Host ("  {0} /{1} -> {2}" -f $req.request.method, $path, $req.name)
        $total++
    }
}
Write-Host ("`nTotal endpoints: {0}" -f $total)
