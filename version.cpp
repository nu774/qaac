const char *get_qaac_version()
{
#ifdef REFALAC
    return "0.34";
#else
    return "1.23";
#endif
}
