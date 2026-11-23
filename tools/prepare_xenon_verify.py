"""Prepare a native verification project and local synthetic H.264/MD5 fixtures."""
from pathlib import Path
import hashlib
import json
import shutil
import xml.etree.ElementTree as ET
import av

ROOT=Path(__file__).resolve().parents[1]
VS=ROOT/'VisualStudio2010'
NS={'m':'http://schemas.microsoft.com/developer/msbuild/2003'}
ET.register_namespace('',NS['m'])


def main():
    tree=ET.parse(VS/'NUVIO.vcxproj')
    root=tree.getroot()
    replacements={
        'ProjectGuid':'{E762A990-0F8F-4C39-90B0-E091CAD5FE77}',
        'ProjectName':'NUVIO-DECODER-VERIFY',
        'TargetName':'NUVIO-DECODER-VERIFY',
        'OutDir':'$(ProjectDir)bin\\XenonVerify\\',
        'IntDir':'$(ProjectDir)obj\\XenonVerify\\',
        'OutputFile':'$(OutDir)NUVIO-DECODER-VERIFY.exe',
        'ImageXexOutput':'$(OutDir)NUVIO-DECODER-VERIFY.xex',
    }
    for node in root.iter():
        tag=node.tag.rsplit('}',1)[-1]
        if tag in replacements:
            node.text=replacements[tag]
    for group in list(root):
        for node in list(group):
            if node.tag.rsplit('}',1)[-1] in ('ClCompile','ProjectReference') and 'Include' in node.attrib:
                group.remove(node)
        if group.tag.rsplit('}',1)[-1]=='Target':root.remove(group)
    sources=ET.SubElement(root,'{'+NS['m']+'}ItemGroup')
    ET.SubElement(sources,'{'+NS['m']+'}ClCompile',Include='tests\\NuvioXenonDecodeVerify.cpp')
    for node in root.findall('.//m:AdditionalIncludeDirectories',NS):
        node.text=(node.text or '')+';$(ProjectDir)ThirdParty\\x360-homebrew-libs\\external\\ffmpeg360;$(ProjectDir)ThirdParty\\x360-homebrew-libs\\external\\ffmpeg360\\vcproj\\external\\include'
    tree.write(VS/'NUVIO-DECODER-VERIFY.vcxproj',encoding='utf-8',xml_declaration=True)
    output=VS/'Release/DecoderVerify'
    output.mkdir(parents=True,exist_ok=True)
    manifest=[]
    for name in ['main-cabac-bf0','main-cabac-bf3']:
        source=ROOT/'ffmpeg360-h264-xenon-main/verify/streams'/(name+'.h264')
        hashes=[]
        with av.open(str(source)) as container:
            for frame in container.decode(video=0):
                assert (frame.width,frame.height,frame.format.name)==(1280,720,'yuv420p')
                digest=hashlib.md5()
                for plane in frame.planes:
                    raw=bytes(plane)
                    for y in range(plane.height):digest.update(raw[y*plane.line_size:y*plane.line_size+plane.width])
                hashes.append(digest.hexdigest())
        assert len(hashes)==300,(name,len(hashes))
        shutil.copy2(source,output/source.name)
        (output/(name+'.expected.md5')).write_text('\n'.join(hashes)+'\n',encoding='ascii')
        manifest.append({'file':source.name,'frames':len(hashes),'sha256':hashlib.sha256(source.read_bytes()).hexdigest()})
    (output/'fixtures.json').write_text(json.dumps({'reference_decoder':av.library_versions,'streams':manifest},indent=2),encoding='utf-8')
    (output/'LEIA-ME.txt').write_text('Copie esta pasta ao Xbox e execute NUVIO-DECODER-VERIFY.xex.\nO teste compara 300 quadros por clip (C e VMX; 1 e 2 threads).\nAguarde alguns minutos e leia xenon-decode-results.txt.\nNao usa conta, rede ou filmes pessoais.\n',encoding='utf-8')
    print('Prepared 2 synthetic streams, 600 desktop reference frame hashes and Xbox verification project.')


if __name__=='__main__':main()
