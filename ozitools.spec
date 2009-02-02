Name:           ozitools
Version:        1.0.0
Release:        1%{?dist}
Summary:        A set of utilities to convert OziExplorer maps to other formats.

Group:          Applications/Productivity
License:        GPL
URL:            http://www.beonway.ru/ozitools
Source0:        %{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  libtiff-devel zlib-devel perl gdal-perl
#Requires:       

%description
Ozitools are useful to convert OziExplorer maps to other
formats, primarily to GeoTiff. ozf2tiff utility converts
ozf2 files to tiff, unfortunately, ozf3 is not supported.
map2geotiff can be used to convert OziExplorer .map file
along with raster data to single georeferenced GeoTiff.

%prep
%setup -q


%build
%configure
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc AUTHORS ChangeLog COPYING README
%_bindir



%changelog
* Mon Jan 26 2009 Mikhail Rumyantsev <dev@beonway.ru>
- Initial build.

