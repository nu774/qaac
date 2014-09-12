const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.43";
#else
    return "2.43";
#endif
}
