# Godot's SVG rasterizer does not draw <text>; bake our label lettering to paths.
Add-Type -AssemblyName System.Drawing
$labelRoot = Join-Path $PSScriptRoot '..\assets\textures\props'
$culture = [System.Globalization.CultureInfo]::InvariantCulture
function Coordinate($point) {
    return $point.X.ToString('0.###', $culture) + ',' + $point.Y.ToString('0.###', $culture)
}
foreach ($labelName in @('staff_badge', 'battery_label')) {
    $labelPath = Join-Path $labelRoot ($labelName + '.svg')
    [xml]$svg = [System.IO.File]::ReadAllText($labelPath)
    foreach ($node in @($svg.GetElementsByTagName('text'))) {
        $style = [System.Drawing.FontStyle]::Regular
        if ($node.GetAttribute('font-weight') -eq 'bold') { $style = [System.Drawing.FontStyle]::Bold }
        $font = [System.Drawing.FontFamily]::new('Arial')
        $size = [single]::Parse($node.GetAttribute('font-size'), $culture)
        $ascent = $size * $font.GetCellAscent($style) / $font.GetEmHeight($style)
        $origin = [System.Drawing.PointF]::new([single]::Parse($node.GetAttribute('x'), $culture), [single]::Parse($node.GetAttribute('y'), $culture) - $ascent)
        $geometry = [System.Drawing.Drawing2D.GraphicsPath]::new()
        $geometry.AddString($node.InnerText, $font, [int]$style, $size, $origin, [System.Drawing.StringFormat]::GenericTypographic)
        $points = $geometry.PathPoints
        $types = $geometry.PathTypes
        $commands = [System.Collections.Generic.List[string]]::new()
        for ($i = 0; $i -lt $points.Length; $i++) {
            switch ($types[$i] -band 7) {
                0 { $commands.Add('M' + (Coordinate $points[$i])) }
                1 { $commands.Add('L' + (Coordinate $points[$i])) }
                3 {
                    $commands.Add('C' + (Coordinate $points[$i]) + ' ' + (Coordinate $points[$i+1]) + ' ' + (Coordinate $points[$i+2]))
                    $i += 2
                }
            }
            if ($types[$i] -band 128) { $commands.Add('Z') }
        }
        $path = $svg.CreateElement('path', 'http://www.w3.org/2000/svg')
        $path.SetAttribute('d', ($commands -join ' '))
        $path.SetAttribute('fill', $node.GetAttribute('fill'))
        [void]$node.ParentNode.ReplaceChild($path, $node)
        $geometry.Dispose()
        $font.Dispose()
    }
    $svg.Save($labelPath)
    Write-Output ('Outlined ' + $labelName)
}
